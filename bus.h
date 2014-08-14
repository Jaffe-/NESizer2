#pragma once

// Address bus: address occupies pins 0, 1 and 2 of PORTB:
#define ADDR_m 0b111
#define ADDR_p 0

// Upper 6 bits of databus in upper 6 bits of PORTD:
#define DATA_PORTD_m 0b11111100

// Lower 2 bits of databus in bits 4, 5 of PORTC:
#define DATA_PORTC_m 0b00000011

// Addresses:
#define CPU_ADDR 0
#define LEDCOL_ADDR 1
#define ROW_ADDR 2
#define SWITCHCOL_ADDR 3
#define MEMORY_ADDRLOW_ADDR 4
#define MEMORY_ADDRMID_ADDR 5
#define MEMORY_ADDRHIGH_ADDR 6
#define MEMORY_DATA_ADDR 7

#define NO_ADDR 7

// Helper functions for using the bus:
#define bus_set_address(ADDRESS)\ 
    PORTB = (PORTB & ~ADDR_m) | ((ADDRESS) << ADDR_p)

#define bus_set_value(VAL)\ 
    PORTD = (PORTD & ~DATA_PORTD_m) | ((VAL) & DATA_PORTD_m);\	       
    PORTC = (PORTC & ~DATA_PORTC_m) | ((VAL) & DATA_PORTC_m)

#define bus_read_value()\
    (PIND & DATA_PORTD_m) | (PINC & DATA_PORTC_m)

#define bus_set_input()\
    PORTD &= ~DATA_PORTD_m; \
    DDRD &= ~DATA_PORTD_m;\
    PORTD |= DATA_PORTD_m;\ 
    PORTC &= ~DATA_PORTC_m;\ 
    DDRC &= ~DATA_PORTC_m;\ 
    PORTC |= DATA_PORTC_m
						   
#define bus_set_output()\ 
    PORTD &= ~DATA_PORTD_m;\ 
    DDRD |= DATA_PORTD_m;\ 
    PORTC &= ~DATA_PORTC_m;\
    DDRC |= DATA_PORTC_m

// Handy nop thingy
#define nop() asm volatile("nop")
