#include "2a03_io.h"
#include <avr/io.h>

#define F_CPU 20000000L
#include <util/delay.h>

#include <util/atomic.h>
#include <avr/interrupt.h>
#include "bus.h"
#include "task.h"
#include "leds.h"

/* 
   2a03_io.c 

   NES APU interface functions
*/

uint8_t io_reg_buffer[0x16] = {0};
uint8_t reg_mirror[0x16] = {0};
uint8_t reg_update[0x16] = {0};

/* Assembly functions in 2a03_asm.S */

extern void register_set12(uint8_t, uint8_t);
extern void register_set15(uint8_t, uint8_t);
extern void register_set16(uint8_t, uint8_t);
extern void reset_pc12(void);
extern void reset_pc15(void);
extern void reset_pc16(void);
extern uint8_t detect(void);

void (*register_set_fn)(uint8_t, uint8_t);
void (*reset_pc_fn)(void);

/* Internal utility functions */

inline void register_write(uint8_t reg, uint8_t value)
/* Write to register
  
   Writes a value to an APU register by feeding the 6502
   with instructions for loading the value into A, and then
   storing the value in $40<reg>.
*/
{
  // Put STA_zp on bus before deactivating CPU latch
  bus_write(STA_zp);

  // Open latch output
  bus_select(CPU_ADDRESS);

  // The code in the register set function has to be cycle exact, so it has to
  // be run with interrupts disabled:
  
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { 
    register_set_fn(reg, value);
  }

  // Finally, latch the last bus value by deselecting the CPU
  bus_deselect();

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

void io_reset_pc()
{
  bus_write(STA_zp);

  bus_select(CPU_ADDRESS);

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    reset_pc_fn();
  }
  
  bus_deselect();
}

uint8_t io_setup()
/* 
   Initializes the interface to communicate with 6502 

   Configures ports and resets the 6502 and puts the APU in a suitable
   (non-interruptive) state.
*/
{
    // Configure the /RES pin on PORT C as output and set /RES high, also set
    // bits 0, 1 as output
    DDRC |= RES;
    PORTC |= RES | RW;

    bus_select(CPU_ADDRESS);

    bus_write(STA_abs);
    
    bus_deselect();
    
    // Set /RES low
    PORTC &= ~RES;

    // Hold it low for some time, for 6502 to perform reset
    _delay_us(10);

    // Set /RES high again
    PORTC |= RES;

    _delay_us(10);

    // Run detect function and set the right functions for communicating with
    // the 2A03/clone chip used
    switch (io_clockdiv = detect()) {
    case 12:
      register_set_fn = &register_set12;
      reset_pc_fn = &reset_pc12;
      break;

    case 15:
      register_set_fn = &register_set15;
      reset_pc_fn = &reset_pc15;
      break;

    case 16:
      register_set_fn = &register_set16;
      reset_pc_fn = &reset_pc16;
      break;

    default:
      return 0;
    }
    
    io_reset_pc();
    return 1;
}
