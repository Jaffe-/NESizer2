#include "constants.h"
#include <avr/io.h>
#include <util/delay.h>
#include "2a03_io.h"
#include "leds.h"
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
    while(PINC & RW);
    while(!(PINC & RW));
    while(PINC & RW);
    while(!(PINC & PHI2));
}

inline void databus_wait()
/*
  Waits for PHI2 to transition from 0 to 1, which signals that the CPU is ready
  to read from the bus. 
*/
{
    // If PHI2 is already 1, we'll wait until it goes low again
    while (PINC & PHI2);

    // Wait until PHI2 transitions from low to high
    while (!(PINC & PHI2));
}

inline void databus_set(uint8_t value)
/* 
   Waits for PHI2 to go high and then puts new value on bus.
*/
{
    databus_wait();
    PORTD = value;
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

void io_register_write(uint8_t reg, uint8_t value)
/* Write to register
  
   Writes a value to an APU register by feeding the 6502
   with instructions for loading the value into A, and then
   storing the value in $4014.
*/
{

    // Ensure that interrupts are disabled when doing this

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
	// Wait for 6502 to reach the third cycle of its idle STA
	// instruction. 
	sync();

	// Do the actual write
	register_set(reg, value);
	
	// Jump back
	reset_pc();
    }

    // Reflect change in mirror
    reg_mirror[reg] = value;
}

void io_register_write_all() 
{
    // Determine which registers needs updating
    for (uint8_t i = 0; i <= 0x15; i++)
	reg_update[i] = (io_reg_buffer[i] != reg_mirror[i]);
	
    // Write to all registers that have been changed
    if (reg_update[0x15]) io_register_write(0x15, io_reg_buffer[0x15]);

    // Square 1
    if (reg_update[0x00]) io_register_write(0x00, io_reg_buffer[0x00]);
    if (reg_update[0x01]) io_register_write(0x01, io_reg_buffer[0x01]);
    if (reg_update[0x02]) io_register_write(0x02, io_reg_buffer[0x02]);
    if (reg_update[0x03]) io_register_write(0x03, io_reg_buffer[0x03]);

    // Square 2
    if (reg_update[0x04]) io_register_write(0x04, io_reg_buffer[0x04]);
    if (reg_update[0x05]) io_register_write(0x05, io_reg_buffer[0x05]);
    if (reg_update[0x06]) io_register_write(0x06, io_reg_buffer[0x06]);
    if (reg_update[0x07]) io_register_write(0x07, io_reg_buffer[0x07]);

    // Noise
    if (reg_update[0x08]) io_register_write(0x08, io_reg_buffer[0x08]);
    if (reg_update[0x0A]) io_register_write(0x0A, io_reg_buffer[0x0A]);
    if (reg_update[0x0B]) io_register_write(0x0B, io_reg_buffer[0x0B]);

    // Triangle
    if (reg_update[0x0C]) io_register_write(0x0C, io_reg_buffer[0x0C]);
    if (reg_update[0x0E]) io_register_write(0x0E, io_reg_buffer[0x0E]);
    if (reg_update[0x0F]) io_register_write(0x0F, io_reg_buffer[0x0F]);

    // DMC: Does not update data register
    if (reg_update[0x10]) io_register_write(0x10, io_reg_buffer[0x10]);
    if (reg_update[0x12]) io_register_write(0x12, io_reg_buffer[0x12]);
    if (reg_update[0x13]) io_register_write(0x13, io_reg_buffer[0x13]);

    // Reset program counter
    reset_pc();

    // Copy buffer over to mirror
    for (uint8_t i = 0; i <= 0x15; i++) 
	reg_mirror[i] = io_reg_buffer[i];
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

void io_setup_2a03()
/* Initializes the interface to communicate with 6502 

   Configures ports and resets the 6502 and puts the APU in a suitable
   (non-interruptive) state.
*/
{
    // Configure all of PORTD as output and set value to 0. This value will be 
    // the taken as the reset vector address the CPU jumps to when restarted. 
    DDRD = 0xFF;
    PORTD = STA_zp;

    // Configure the /RES pin on PORT C as output and set /RES high
    DDRC = RES;
    PORTC = RES | RW | PHI2;

    _delay_ms(50);

    // Reset the 2A03
    reset_2a03();

    // Wait for reset cycle to complete
    _delay_ms(1);
    
    // The CPU should now be executing instructions. Instruct it to jump to 
    // address 0. This way the addresses can be monitored. 
    sync();

    // Send SEI instruction
    databus_set(0x78);
    databus_wait();

    reset_pc();

    databus_set(STA_zp);

    // Now the 6502 should be ready to receive instructions (?)

    // We need to disable the frame interrupt
    io_register_write(0x17, 0b01000000);

    // Ensure that DMC channel does not trigger IRQ
    io_register_write(0x15, 0);
}

