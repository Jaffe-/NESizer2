#include "2a03_io.h"
#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>
#include "bus.h"

/* 
   2a03_io.c 

   NES APU interface functions
*/

uint8_t io_reg_buffer[0x16] = {0};
uint8_t reg_mirror[0x16] = {0};
uint8_t reg_update[0x16] = {0};

/* Internal utility functions */

extern void register_set12(uint8_t, uint8_t);
extern void register_set15(uint8_t, uint8_t);

inline void register_write(uint8_t reg, uint8_t value)
/* Write to register
  
   Writes a value to an APU register by feeding the 6502
   with instructions for loading the value into A, and then
   storing the value in $40<reg>.
*/
{
    // Ensure that interrupts are disabled when doing this

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
	// Put STA_zp on bus before deactivating CPU latch
	bus_write(STA_zp);
	
	// Open latch gates
	bus_select(CPU_ADDRESS);

	// Wait for 6502 to reach the third cycle of its idle STA
	// instruction. 

	register_set15(reg, value);

	// Finally, latch the bus value by switching to a
	// different address
	bus_deselect();
    }

    // Reflect change in mirror
    reg_mirror[reg] = value;
}


/* External functions */

void io_register_write(uint8_t reg, uint8_t value)
{
    register_write(reg, value);
}

void io_write_changed(uint8_t reg) 
{
    if (io_reg_buffer[reg] != reg_mirror[reg]) {
	register_write(reg, io_reg_buffer[reg]);
	reg_mirror[reg] = io_reg_buffer[reg];
    }
}

void io_setup()
/* Initializes the interface to communicate with 6502 

   Configures ports and resets the 6502 and puts the APU in a suitable
   (non-interruptive) state.
*/
{
    // Configure the /RES pin on PORT C as output and set /RES high, also set
    // bits 0, 1 as output
    DDRC |= RES;
    PORTC |= RES | RW;

    bus_write(STA_zp);

    // Reset the 2A03:

    // Set /RES low
    PORTC &= ~RES;

    // Hold it low for some time, for 6502 to perform reset
    _delay_ms(10);

    // Set /RES high again
    PORTC |= RES;

    // Wait for reset cycle to complete
    _delay_ms(1);

    //io_register_write(0x17, 0);

}

