/**
 * @file main.c
 * @author Tzu-Yun Chang
 * @brief Embedded Network System Coursework
 * @version 0.1
 * @date 2022-12-22
 * 
 * @copyright Copyright (c) 2022
 * 
 * This program involves the applciation, application layer, and transport layer.
 * 
 */


#include "../include/uart.h"
#include "../include/header.h"
#include "../include/hal.h"
#include "../include/transport_layer.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>


/**
 * 
 * --------------------------------------------------------------------------------------------------
 * |  TEST_CASE     | Description								    |
 * --------------------------------------------------------------------------------------------------
 * |       0        | Send data but no ACK received, the device timeout and retransmit the segment  |
 * |       1        | Send data and an ACK is received, correct checksum			    |
 * |       2        | Send data and an ACK is received, incorrect checksum			    |
 * |       3        | Send data and an ACK is received, incorrect sequence			    |
 * |       4        | Received a segment, not an ACK with correct checksum			    |
 * |       5        | Received a segment, not an ACK with incorrect checksum		    	    |
 * --------------------------------------------------------------------------------------------------
 */

#define TEST_CASE 5
#define DEVICE_NUMBER 1

static transport_queue trans_queue;
static application app;

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
	put_str("\r\nInitiaizing\r\n\r\n");
	trans_queue = create_transport_queue(20);
	device_setup();

	transport rx_trans, tx_trans;
	tl_segment segment;
	uint8_t dest, net_receive, ack_seq;
	rx_trans.update_app = rx_trans.send_ack = 0;
	net_receive = 0;
	ack_seq = 0;

	while (1)
	{
		if (app.transmit_flag)
		{
			put_str("\r\n******************************");
			put_str("\n\r**  APP :   BUTTON PRESSED  **\n\r");
			put_str("******************************\r\n");
			transport_enqueue(&trans_queue, TL_send(app));
			app.transmit_flag = false;
		}

		if(rx_trans.update_app)
		{
			put_str("\n\rAPP :   UPDATE LED\n\r");
            set_pwm(app.rx_data);
			rx_trans.update_app = false;
		}

		if (!transport_queue_isempty(&trans_queue) && !read_timer_status())
		{
			put_str("\r\n--------------------------------");
			put_str("\n\r|   TRANS:   TRANSMITTING      |\r\n");
			put_str("--------------------------------\r\n");
			char buf[5];
			put_str("Buffer Size : ");
			put_str(itoa(transport_queue_size(&trans_queue), buf, 5));
			put_str("\r\n");
			tx_trans = transport_dequeue(&trans_queue);
			enable_timeout();
			_delay_ms(10);

#if (TEST_CASE == 1 || TEST_CASE == 2 || TEST_CASE == 3)
	// Packaging an ACK
	uint8_t checksum, i;
	dest = 0;
	net_receive = 1;
	segment.buf = (uint8_t *)malloc((tx_trans.buf.len)*sizeof(uint8_t));
    segment.len = tx_trans.buf.len;
	segment.buf[0] = ack_seq;
    segment.buf[1] = tx_trans.buf.buf[0];
    segment.buf[2] = tx_trans.buf.buf[3];
    segment.buf[3] = tx_trans.buf.buf[2];
    segment.buf[4] = tx_trans.buf.buf[4];
    for (i = 0; i < 2; ++i)
    {
        segment.buf[i+5] = 0;
	}
    checksum = sum_checksum(segment.len-2, segment.buf);
	segment.buf[segment.len-2] = (uint8_t)(checksum >> 4);
    segment.buf[segment.len-1] = (uint8_t)checksum;
	++ack_seq;
#if (TEST_CASE == 2)
	// Incorrect checksum
	++segment.buf[segment.len-1];
#elif (TEST_CASE == 3)
	// Incorrect sequence
	++segment.buf[1];
#endif

#elif (TEST_CASE == 4 || TEST_CASE == 5)
	disable_timeout();
	dest = 0;
	net_receive = 1;
#if (TEST_CASE == 5)
	++tx_trans.buf.buf[tx_trans.buf.len-1];
#endif
#endif
		} else if(tl_retransmit_flag){
			char buf[5];
			put_str("Timer status : ");
			put_str(itoa(read_timer_status(), buf, 5));
			put_str("\r\n");
			put_str("\n\rTRANS:   RE-TRANSMIT\n\r");
			increment_attempts();
			tl_retransmit_flag = false;
		} 
		
		if (rx_trans.send_ack)
		{
			put_str("\n\rTRANS:   SEND ACK\n\r");
			rx_trans.send_ack = false;
			increment_rxsequence();
		}

		if (net_receive)
		{
			put_str("\n\rNET:   SEND ACK\n\r");
			net_receive = 0;
#if (TEST_CASE == 4 || TEST_CASE == 5)
	TL_receive(dest, &tx_trans, &rx_trans, &app.rx_data);
#else
	TL_receive(dest, &segment, &rx_trans, &app.rx_data);
#endif
		}
	}
}
