#pragma once

// Address bus: address occupies pins 0, 1 and 2 of PORTB:
#define ADDR_m 0b111
#define ADDR_p 0

// Upper 6 bits of databus in upper 6 bits of PORTD:
#define DATA_PORTD_m 0b11111100
#define DATA_PORTD_t 0b00000011

// Lower 2 bits of databus in bits 4, 5 of PORTC:
#define DATA_PORTC_m 0b00000011
#define DATA_PORTC_t 0b11111100

// Addresses:
#define CPU_ADDR 0
#define LEDCOL_ADDR 1
#define ROW_ADDR 2
#define SWITCHCOL_ADDR 3
#define ATTINY_ADDR 4
#define NO_ADDR 7

// Helper functions for using the bus:
#define bus_set_address(ADDR) PORTB = (PORTB & ~ADDR_m) | (ADDR << ADDR_p)

//#define bus_set_value(VAL) PORTD = (PORTD & DATA_PORTD_t) | ((VAL) & DATA_PORTD_m); \ 
//                           PORTC = (PORTC & DATA_PORTC_t) | ((VAL) & DATA_PORTC_m)  

/*
#define bus_set_value(VAL) 
asm volatile("in $0, 0x0b"\
	     "andi $0, 0x03"			\
	     "ldi $0, " #VAL			\
	     "andi $1, 0xFC"			\
	     "or $0, $1"			\
	     "in $1, 0x08"			\
	     "andi $1, 0xFC"			\
	     "ldi $2, " #VAL			\
	     "andi $2, 0x03"			\
	     "or $1, $2"			\
	     "out 0x0b, $0"			\
	     "out 0x08, $1")
*/

#define bus_set_value(VAL)					\
    asm ("mov r26, %[value]\n"				\
		 "in r24, 0x0b\n"				\
		 "andi r24, 0x03\n"				\
		 "andi %[value], 0xFC\n"			\
		 "or r24, %[value]\n"				\
		 "in r25, 0x08\n"				\
		 "andi r25, 0xFC\n"				\
		 "andi r26, 0x03\n"				\
		 "or r25, r26\n"				\
		 "out 0x0b, r24\n"				\
		 "out 0x08, r25\n"				\
		 :							\
		 : [value] "r" ((VAL))					\
		 : "r24", "r25", "r26")

#define bus_read_value() (PIND & DATA_PORTD_m) | (PINC & DATA_PORTC_m)

#define bus_set_input()\
    PORTD &= ~DATA_PORTD_m; DDRD &= ~DATA_PORTD_m; PORTD |= DATA_PORTD_m;\ 
    PORTC &= ~DATA_PORTC_m; DDRC &= ~DATA_PORTC_m; PORTC |= DATA_PORTC_m
						   

#define bus_set_output() PORTD &= ~DATA_PORTD_m; DDRD |= DATA_PORTD_m; PORTC &= ~DATA_PORTC_m; DDRC |= DATA_PORTC_m

// Handy nop thingy
#define nop() asm volatile("nop")
