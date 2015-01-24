/* NESIZER
   Bus interface

   (c) Johan Fjeldtvedt

   Handles low level bus communication. The bus consists of three address lines 
   and eight data lines.  
 */
#pragma once

#include <avr/io.h>

// Address bus: address occupies pins 0, 1 and 2 of PORTB:
#define ADDR_m 0b111
#define ADDR_p 0

// Upper 6 bits of databus in upper 6 bits of PORTD:
#define DATA_PORTD_m 0b11111100

// Lower 2 bits of databus in bits 4, 5 of PORTC:
#define DATA_PORTC_m 0b00000011

// Address select bit is bit 1 of PORTD:
#define BUS_EN_m 0b00000010

// Addresses for the various components on the bus. These are hardware determined.
#define CPU_ADDRESS 0
#define LEDCOL_ADDRESS 1
#define ROW_ADDRESS 2
#define SWITCHCOL_ADDRESS 3
#define MEMORY_LOW_ADDRESS 4
#define MEMORY_MID_ADDRESS 5
#define MEMORY_HIGH_ADDRESS 6

// Selects a component on the bus by sending the address to the decoder and then
// enabling the decoder's outputs
#define bus_select(ADDRESS)		               \
    PORTB = (PORTB & ~ADDR_m) | ((ADDRESS) << ADDR_p); \
    PORTD |= BUS_EN_m

// Deactivates the currently selected component by deactivating the decoder's
// output
#define bus_deselect()	\
    PORTD &= ~BUS_EN_m

// Puts VAL on the bus
#define bus_write(VAL)					      \
    PORTC = (PORTC & ~DATA_PORTC_m) | ((VAL) & DATA_PORTC_m); \
    PORTD = (PORTD & ~DATA_PORTD_m) | ((VAL) & DATA_PORTD_m)	

// Reads from the bus
#define bus_read() \
    (PIND & DATA_PORTD_m) | (PINC & DATA_PORTC_m)

// Configures the pins of PORTC and PORTD connected to the bus to be input pins. 
#define bus_dir_input()	                        \
    PORTD &= ~DATA_PORTD_m;			\
    DDRD &= ~DATA_PORTD_m;			\
    PORTD |= DATA_PORTD_m;                      \
    PORTC &= ~DATA_PORTC_m;                     \
    DDRC &= ~DATA_PORTC_m;                      \
    PORTC |= DATA_PORTC_m

// Configures the pins of PORTC and PORTD connected to the bus to be output pins
#define bus_dir_output()                        \
    PORTD &= ~DATA_PORTD_m;                     \
    DDRD |= DATA_PORTD_m;                       \
    PORTC &= ~DATA_PORTC_m;			\
    DDRC |= DATA_PORTC_m

// Simply injects a nop instruction, handy for waiting one clock cycle (for
// example for a write cycle to complete)
#define nop() asm volatile("nop")

// Setup function
void bus_setup();
