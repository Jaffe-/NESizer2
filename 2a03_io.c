#include "constants.h"
#include <avr/io.h>
#include <util/delay.h>
#include "2a03_io.h"
#include "leds.h"
#include "bus.h"
#include <util/atomic.h>

/* 
   2a03_io.c 

   NES APU interface functions
*/

uint8_t io_reg_buffer[0x16] = {0};
uint8_t reg_mirror[0x16] = {0};
uint8_t reg_update[0x16] = {0};

/* Internal utility functions */

inline void sync() 
/*
  Synchronizes with the CPU by waiting for the R/W line to go high. When this
  happens, we know that it is executing the third cycle of the STA_zp
  instruction (it is writing the bogus value to memory). 
 */
{
    while(PINC & RW);       // wait until R/W goes low
    while(!(PINC & RW));    // wait until it goes up again
    while(PINC & RW);       // wait until R/W goes low
}

inline void register_set(uint8_t reg, uint8_t val) 
/* 
   Writes value to desired APU register by sequentially sending an LDA 
   instruction to transfer the value to the accumulator, and then sending an STA 
   instruction to store the accumulator in the register's address

   Assumes that synchronization is done, and that the idle instruction is put
   back on the bus afterwards.
*/
{
    // Wait for 6502's read and put LDA_imm on databus:
    bus_set_value(LDA_imm);         // takes 2 cycles
    bus_set_value(val);

    bus_set_value(STA_abs);         // takes 4 cycles
    bus_set_value(reg);
    bus_set_value(0x40);

    // Put back STA zero page instruction
    bus_set_value(STA_zp);
}


/* External functions */

inline void io_register_write(uint8_t reg, uint8_t value)
/* Write to register
  
   Writes a value to an APU register by feeding the 6502
   with instructions for loading the value into A, and then
   storing the value in $4014.
*/
{
    // Ensure that interrupts are disabled when doing this

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
	// Put STA_zp on bus before deactivating CPU latch
	bus_set_value(STA_zp);
	
	// Open latch gates
	bus_set_address(CPU_ADDR);

	// Wait for 6502 to reach the third cycle of its idle STA
	// instruction. 
	sync();

	// Do the actual write
	register_set(reg, value);
	
	// Finally, latch the bus value by switching to a
	// different address
	bus_set_address(NO_ADDR);
    }

    // Reflect change in mirror
    reg_mirror[reg] = value;
}

void io_write_changed(uint8_t reg) 
{
    if (io_reg_buffer[reg] != reg_mirror[reg]) {
	io_register_write(reg, io_reg_buffer[reg]);
	reg_mirror[reg] = io_reg_buffer[reg];
    }
}

void io_setup()
/* Initializes the interface to communicate with 6502 

   Configures ports and resets the 6502 and puts the APU in a suitable
   (non-interruptive) state.
*/
{
    // Configure upper 6 bits of PORTD as output:
    PORTD = 0;
    DDRD = DATA_PORTD_m;

    // Configure the /RES pin on PORT C as output and set /RES high, also set
    // bits 0, 1 as output
    PORTC = 0;
    DDRC = RES | DATA_PORTC_m;
    PORTC = RES | RW;

    bus_set_value(STA_zp);

    // Reset the 2A03:

    // Set /RES low
    PORTC &= ~RES;

    // Hold it low for some time, for 6502 to perform reset
    _delay_ms(10);

    // Set /RES high again
    PORTC |= RES;

    // Send SEI instruction:
    sync();
    bus_set_value(0x78);
    bus_set_value(STA_zp);
    
    // Wait for reset cycle to complete
    _delay_ms(1);
        
}

