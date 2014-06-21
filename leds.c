#include <avr/io.h>
#include "leds.h"

/* 
   Status LED interface 

   The 8 status LEDs are connected to a 74HC164 shift register.
   The shift register is connected to the Atmega using the SPI interface:

   PB3 (MOSI) is connected to the data inputs
   PB5 (SCK) is connected to the clock input
*/

void leds_setup()
{
    // Set MISO/SHIFT_DATA, SCK/SHIFT_CLK and SS as output pins.
    // SS (PB2) must be set to output even though it is never used. 
    DDRB |= SHIFT_DATA | SHIFT_CLK | SS;
    
    // Configure SPI (no interrupts, master, MSB first
    // f_cpu / 4 as clock frequency
    SPCR = 0b01010000;
}

void set_leds(uint8_t val)
{
    // Writing to the SPI data register triggers transfer:
    SPDR = val;
}

