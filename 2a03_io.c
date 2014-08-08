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

inline void databus_wait();

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
//    while(!(PINC & RW));    // wait until it goes up again
//    while(PINC & RW);       
    while(!(PINC & PHI2));
//    while(PINC & PHI2);
    nop();
    nop();

}

inline void databus_wait()
/*
  Waits for PHI2 to transition from 0 to 1, which signals that the CPU is ready
  to read from the bus. 
*/
{
//    nop();
}

inline void databus_set(uint8_t value)
/* 
   Waits for PHI2 to go high and then puts new value on bus.
*/
{
    bus_set_value(value);
    databus_wait();
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
    databus_set(LDA_imm);         // takes 2 cycles
    databus_set(val);

    databus_set(STA_abs);         // takes 4 cycles
    databus_set(reg);
    databus_set(0x40);
    databus_wait();    
}

inline void reset_pc() 
/* 
   Resets PC to avoid CPU reading from APU registers when fetching instructions. 

   This function assumes that synchronization is done. 
*/
{
    // Send JMP 0x4018 instruction:
    databus_set(JMP_abs);
    databus_set(0x18);
    databus_set(0x40);

    // Send default LDA instruction
    databus_set(STA_zp);
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
	
	// Jump back
	reset_pc();

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

void reset_2a03()
/* Simply resets the 6502. */
{
    // Set /RES low
    PORTC &= ~RES;

    // Hold it low for some time, for 6502 to perform reset
    _delay_ms(10);

    // Set /RES high again
    PORTC |= RES;
}

void io_setup()
/* Initializes the interface to communicate with 6502 

   Configures ports and resets the 6502 and puts the APU in a suitable
   (non-interruptive) state.
*/
{
    PORTD = 0;
    DDRD = DATA_PORTD_m;

    // Configure the /RES pin on PORT C as output and set /RES high
    PORTC = 0;
    DDRC = RES | DATA_PORTC_m;
    PORTC = RES | RW | PHI2;

    bus_set_value(STA_zp);

    _delay_ms(50);

    // Reset the 2A03
    reset_2a03();

    // Wait for reset cycle to complete
    _delay_ms(1);
    
    sync();

    // Send SEI instruction
    databus_set(0x78);
    databus_wait();

    reset_pc();
    
    // Now the 6502 should be ready to receive instructions (?)

    // We need to disable the frame interrupt
    io_register_write(0x17, 0b01000000);

    // Ensure that DMC channel does not trigger IRQ
    io_register_write(0x15, 0);

}

