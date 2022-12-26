# Modified by Domenico Balsamo

TRG	= app
SRC	= source/main.c source/uart.c source/transport_layer.c source/hal.c

PRGER		= usbasp
MCU_TARGET	= atmega644p
MCU_FREQ	= 12000000

TFT_HARDWARE	= ili9341_parallel
TFT_PORT_CTRL	= A
TFT_PORT_DATA	= C

DEFS	+= -D__PLATFORM_AVR__
LIBS	+= -lm

EFUSE	= 0xFF
HFUSE	= 0x9C
LFUSE	= 0xE7

include Makefile_AVR.defs


