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
  Performs a write to an APU register by sending an LDA <value> instruction, then an 
  STA <reg> instruction, and finally putting the idle STA zero-page instruction back. 

  This function has been changed a lot from earlier revisions in that it now is written
  purely in assembly. This was necessary to make register writes reliable when the databus
  is split across PORTC and PORTD. 

  The sync() function is done about 2-3 clock cycles after R/W actually goes low. The setup code
  for the following asm statement (putting values in register etc.) takes 6 cc. Thus the 
  first value is put on the bus about one 6502 ccs after R/W went low. That is safely within the
  valid bus value window. The other writes are padded out with NOPs so that there is exactly 12 
  cc between each register write (so that each write happens at the same phase of the 6502 cc)
 */
{

    asm volatile(
	// Write LDA_imm to 6502:
	"mov r24, %[portd_reg]\n"      // get PORTD with upper 6 bits cleared
	"ori r24, %[lda_imm_hi]\n"     // or with upper 6 bits of value
	"mov r25, %[portc_reg]\n"      // get PORTC with lower 2 bits cleared
	"ori r25, %[lda_imm_lo]\n"     // or with lower 2 bits of value
	"out %[portd_addr], r24\n"     // output the values
	"out %[portc_addr], r25\n"
	
	// Write value:
	"mov r24, %[portd_reg]\n"      // get PORTD with upper 6 bits cleared
	"mov r26, %[v]\n"
	"andi r26, 0xFC\n"             
	"or r24, r26\n"                // or with upper 6 bits of value
	"mov r25, %[portc_reg]\n"      // get PORTC with lower 2 bits cleared
	"mov r26, %[v]\n"
	"andi r26, 0x03\n"
	"or r25, r26\n"                // or with lower 2 bits of value
	"nop\n"                        // waste two cycles so that the new value is output
	"nop\n"                        // exactly 12 cc after the last time
	"out %[portd_addr], r24\n"     // output the values
	"out %[portc_addr], r25\n"
	
	// Write STA_abs:
	"mov r24, %[portd_reg]\n"
	"ori r24, %[sta_abs_hi]\n"
	"mov r25, %[portc_reg]\n"
	"ori r25, %[sta_abs_lo]\n"
	"nop\n"                        // again, waste cycles so that the next write happens
	"nop\n"                        // 12 cc (one 6502 cc) after the last one
	"nop\n"
	"nop\n"
	"nop\n"
	"nop\n"
	"out %[portd_addr], r24\n"
	"out %[portc_addr], r25\n"
	
	// Write low byte of register:
	"mov r24, %[portd_reg]\n"
	"mov r26, %[r]\n"
	"andi r26, 0xFC\n"
	"or r24, r26\n"
	"mov r25, %[portc_reg]\n"
	"mov r26, %[r]\n"
	"andi r26, 0x03\n"
	"or r25, r26\n"
	"nop\n"
	"nop\n"
	"out %[portd_addr], r24\n"
	"out %[portc_addr], r25\n"
	
	// Write high byte of register:
	"mov r24, %[portd_reg]\n"
	"ori r24, 0x40\n"
	"mov r25, %[portc_reg]\n"
	"nop\n"
	"nop\n"
	"nop\n"
	"nop\n"
	"nop\n"
	"nop\n"
	"nop\n"
	"out %[portd_addr], r24\n"
	"out %[portc_addr], r25\n"

	// Put STA_zp back on the bus:
	"mov r24, %[portd_reg]\n"
	"ori r24, %[sta_zp_hi]\n"
	"mov r25, %[portc_reg]\n"
	"ori r25, %[sta_zp_lo]\n"
	"nop\n"
	"nop\n"
	"nop\n"
	"nop\n"
	"nop\n"
	"nop\n"
	"out %[portd_addr], r24\n"
	"out %[portc_addr], r25\n"
	
	:
	: [portd_addr] "I" (_SFR_IO_ADDR(PORTD)), 
	  [portc_addr] "I" (_SFR_IO_ADDR(PORTC)),
	  [portd_reg] "r" (PORTD & 0x03), 
	  [portc_reg] "r" (PORTC & 0xFC),
	  [v] "r" (val),
	  [r] "r" (reg),
	  [lda_imm_hi] "M" (LDA_imm & 0xFC),
	  [lda_imm_lo] "M" (LDA_imm & 0x03),
	  [sta_abs_hi] "M" (STA_abs & 0xFC),
	  [sta_abs_lo] "M" (STA_abs & 0x03),
	  [sta_zp_hi] "M" (STA_zp & 0xFC),
	  [sta_zp_lo] "M" (STA_zp & 0x03)
	: "r24", "r25", "r26");
}


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

    // Wait for reset cycle to complete
    _delay_ms(1);

    // Send SEI instruction:
    sync();

    asm volatile(
	// Write SEI to 6502:
	"mov r24, %[portd_reg]\n"      // get PORTD with upper 6 bits cleared
	"ori r24, 0x78\n"              // or with upper 6 bits of value
	"mov r25, %[portc_reg]\n"      // get PORTC with lower 2 bits cleared
	"nop\n"
	"out %[portd_addr], r24\n"     // Output these values
	"out %[portc_addr], r25\n"

	// Write STA_zp:
	"mov r24, %[portd_reg]\n"
	"ori r24, %[sta_zp_hi]\n"
	"mov r25, %[portc_reg]\n"
	"ori r25, %[sta_zp_lo]\n"
	"nop\n"
	"nop\n"
	"nop\n"
	"nop\n"
	"nop\n"
	"nop\n"
	"out %[portd_addr], r24\n"
	"out %[portc_addr], r25\n"
		
	:
	: [portd_addr] "I" (_SFR_IO_ADDR(PORTD)), 
	  [portc_addr] "I" (_SFR_IO_ADDR(PORTC)),
	  [portd_reg] "r" (PORTD & 0x03), 
	  [portc_reg] "r" (PORTC & 0xFC),
	  [sta_zp_hi] "M" (STA_zp & 0xFC),
	  [sta_zp_lo] "M" (STA_zp & 0x03)
	: "r24", "r25"
    );
        
}

