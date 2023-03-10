

#include "NET.h"
#include "stdbool.h"
#include "uart.h"
#include "header.h"
#include "hal.h"
#include "transport_layer.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>

#define TEST_SCENARIO 1
#define TEST_TRANSPORT 1
#define DEVICE_NUMBER 1


static Network pack;
static transport_queue trans_queue;
static application app;

uint8_t TRAN_Segment[9] = {0xFF, 0x00, 0x01, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00};
uint8_t Packet_SC1[16] = {0xFF, 0x00, 0x01, 0x02, 0x09, 0xFF, 0x00, 0x01, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00};
uint8_t BROADCAST_PACKET[9] = {0xFF, 0x00, 0x03, 0xFF, 0x02, 0x00, 0x00, 0x00, 0x00};
uint8_t REBROADCAST_PACKET1[9] = {0xFE, 0x01, 0x03, 0xFF, 0x02, 0x00, 0x00, 0x00, 0x00};
uint8_t REBROADCAST_PACKET2[9] = {0xFE, 0x01, 0x03, 0xFF, 0x02, 0x00, 0x00, 0x00, 0x00};
uint8_t HOP_PACKET[16] = {0xFF, 0x00, 0x02, 0x03, 0x09, 0xFF, 0x00, 0x01, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00};
uint8_t AT_HOME[16] = {0xFE, 0x00, 0x02, 0x01, 0x09, 0xFF, 0x00, 0x01, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00};



ISR (INT0_vect)
{
    app.src_port = 0; // button 0
    app.dest_port = 2;
#if (DEVICE_NUMBER == 0)
    app.dest_dev = 1;
#elif (DEVICE_NUMBER == 1)
    app.dest_dev = 0;
#elif (DEVICE_NUMBER == 2)
    app.dest_dev = 1;
#endif
    app.tx_data = read_adc();
    app.transmit_flag = 1;
}


ISR (INT1_vect)
{
    app.src_port = 1; // button 1
    app.dest_port = 2;
#if (DEVICE_NUMBER == 0)
    app.dest_dev = 2;
#elif (DEVICE_NUMBER == 1)
    app.dest_dev = 2;
#elif (DEVICE_NUMBER == 2)
    app.dest_dev = 0;
#endif
    app.tx_data = read_adc();
    app.transmit_flag = 1;
}


int main()
{
	init_uart0();
	put_str("\n\rInitiating\r\n\r\n\r\n");
	trans_queue = create_transport_queue(20);
	device_setup();
    transport rx_trans, tx_trans;
    tl_segment rx;
	rx_trans.update_app = rx_trans.send_ack = 0;
	bool DLL_sendup1 = false, DLL_sendup2 = false, DLL_sendup3 = false;
	
	while(1)
	{
		/* APP INPUTS */
		if (app.transmit_flag)
		{
			put_str("\n\rAPP  :   BUTTON PRESSED\n\r");
			transport_enqueue(&trans_queue, TL_send(app));
			app.transmit_flag = false;
		}
		if(rx_trans.update_app)
		{
			put_str("\n\rAPP  :   UPDATE LED\n\r");
            set_pwm(app.rx_data);
		}
		if (!transport_queue_isempty(&trans_queue) && !read_timer_status())
		{
			put_str("\n\rTRANS:   TRANSMIT\n\r");
			tx_trans = transport_dequeue(&trans_queue);
			enable_timeout();
			NET_receive_TRAN(&pack, tx_trans.buf.buf, tx_trans.buf.len, tx_trans.dev);
			_delay_ms(10);
		} else if(tl_retransmit_flag){
			put_str("\n\rTRANS:   RE-TRANSMIT\n\r");
			increment_attempts();
			NET_receive_TRAN(&pack, tx_trans.buf.buf, tx_trans.buf.len, tx_trans.dev);
			tl_retransmit_flag = false;
		} 
		
		if (rx_trans.send_ack)
		{
			put_str("\n\rTRANS:   SEND ACK\n\r");
			NET_receive_TRAN(&pack, rx_trans.buf.buf, rx_trans.buf.len, rx_trans.dev);
			rx_trans.send_ack = false;
			increment_rxsequence();
		}
		
		if(pack.broadcast){
			/*DLL_receive(&dll, pack.broadpack, pack.packet_len, 0xFF);*/
			put_str("\n\rNET  :   BROADCASTING\n\r");
			//LLC_NET_Interace(0xFF, pack.broadpack, 9, PASS_FRAME);   
			pack.broadcast = false;
			/*pack.sPacket[0] = 0xFE;*/
			//pack.sPacket[2] = 2;
			//pack.sPacket[3] = 1;
			//NET_receive_DLL(&pack, pack.sPacket, 2, 16);
			DLL_sendup1 = true;
		}
		else if(pack.rebroadcast){
			/*DLL_receive(&dll, pack.broadpack, pack.packet_len, 0xFF);*/
			put_str("\n\rNET  :   REBROADCASTING\r\n");
			//LLC_NET_Interace(0xFF, pack.broadpack, 9, PASS_FRAME);
			pack.rebroadcast = false;
			DLL_sendup2 = true;
		}
		else if(pack.send){
			/*DLL_receive(&dll, pack.sPacket, pack.packet_len, dest_mac_address);*/
			put_str("\n\rNET  :   SENDING DATA\r\n");
			//LLC_NET_Interace(pack.dest_mac_address, pack.sPacket, pack.packet_len, PASS_FRAME);
			pack.send = false;
			DLL_sendup3 = true;
		}
		/* else if(DLL flag to send up to NET){
			/* NET_receive_DLL(&pack, dll.packet, dll.src_mac_address, dll.pack_len);
		 }*/
		 else if(DLL_sendup1){
			 NET_receive_DLL(&pack, BROADCAST_PACKET, 3, 9);
			 DLL_sendup1 = false;
		 }
		 else if(DLL_sendup2){
			 NET_receive_DLL(&pack, REBROADCAST_PACKET1, 2, 9);
			 DLL_sendup2 = false;
		 }
		 else if(DLL_sendup3){
			 pack.sPacket[2] = 3;
			 pack.sPacket[3] = 1;
			 NET_receive_DLL(&pack, pack.sPacket, 3, 16);
		 }
		else if(pack.receive){
			put_str("\n\rNET  :   RECEIVING");
			tl_segment segment;
			segment.buf = pack.TRAN_Segment;
			segment.len = pack.seg_length;
			TL_receive(pack.DEST, &segment, &rx_trans, &app.rx_data);
			pack.receive = false;
		}
			
	}
}