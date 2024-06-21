/*
  Copyright 2014-2017 Johan Fjeldtvedt

  This file is part of NESIZER.

  NESIZER is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  NESIZER is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NESIZER.  If not, see <http://www.gnu.org/licenses/>.



  Low level 2A03 I/O interface

  Contains functions for communicating with and controlling the
  2A03.
*/

#include "2a03.h"
#include <avr/io.h>
#include <stdbool.h>

#include <util/delay.h>

#include <util/atomic.h>
#include <avr/interrupt.h>
#include "io/bus.h"
#include "task/task.h"
#include "modulation/periods.h"

// The 6502 opcodes needed
#define LDA_imm 0xA9
#define STA_abs 0x8D
#define STA_zp 0x85
#define JMP_abs 0x4C

// Pins used to interface with 6502
#define RES 0b100             // PC2
#define RW 0b1000          // PC3

uint8_t io_reg_buffer[0x18];
uint8_t reg_mirror[0x18];

/* Assembly functions in 2a03_asm.s */

extern void register_set12(uint8_t, uint8_t);
extern void register_set15(uint8_t, uint8_t);
extern void register_set16(uint8_t, uint8_t);
extern void reset_pc12(void);
extern void reset_pc15(void);
extern void reset_pc16(void);
extern void disable_interrupts12(void);
extern void disable_interrupts15(void);
extern void disable_interrupts16(void);
extern uint8_t detect_2a03_type(void);

void (*register_set)(uint8_t, uint8_t);
void (*reset_pc)(void);
void (*disable_interrupts)(void);

uint8_t io_clockdiv;

/* Internal utility functions */

/* Write to register

   Writes a value to an APU register by feeding the 6502
   with instructions for loading the value into A, and then
   storing the value in $40<reg>.
*/
static inline void register_write(uint8_t reg, uint8_t value)
{
    // Put STA_zp on bus before deactivating CPU latch
    bus_write(STA_zp);

    // Open latch output
    bus_select(CPU_ADDRESS);

    // The code in the register set function has to be cycle exact, so it has to
    // be run with interrupts disabled:

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        register_set(reg, value);
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

        if (reg == 0x03 || reg == 0x07) {

            /* Trick for avoiding phase reset when changing high bits of timer period */

            uint8_t low_val = reg_mirror[reg - 1];
            if ((reg_mirror[reg] & 0x07) - (io_reg_buffer[reg] & 0x07) == 1) {
                register_write(reg - 1, 0);    // low value = 0
                register_write(reg - 2, 0x8F); // enable sweep, negate, shift = 7
                register_write(0x17, 0xC0); // clock sweep immediately
                register_write(reg - 2, 0x0F); // disable sweep
                register_write(reg - 1, low_val); // put back low value
                reg_mirror[reg - 1] = low_val;
                reg_mirror[reg] = io_reg_buffer[reg];
                return;
            }

            else if ((io_reg_buffer[reg] & 0x07) - (reg_mirror[reg] & 0x07) == 1) {
                register_write(reg - 1, 0xFF);
                register_write(reg - 2, 0x87); // enable sweep, negate, shift = 7
                register_write(0x17, 0xC0); // clock sweep immediately
                register_write(reg - 2, 0x0F); // disable sweep
                register_write(reg - 1, low_val); // put back low value
                reg_mirror[reg - 1] = low_val;
                reg_mirror[reg] = io_reg_buffer[reg];
                return;
            }
        }

        register_write(reg, io_reg_buffer[reg]);
    }
}

void io_reset_pc(void)
{
    bus_write(STA_zp);

    bus_select(CPU_ADDRESS);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        reset_pc();
    }

    bus_deselect();
}

static uint8_t rw_transition_count;

ISR(PCINT1_vect)
{
    if (rw_transition_count < 255)
        rw_transition_count++;
}

bool detect_2a03_presence(void)
{
    rw_transition_count = 0;

    /* Enable Pin Change Interrupt 1 and unmask PCINT11 (Port C pin 3, RW signal from 2A03) */
    PCMSK1 = (1 << PCINT11);
    PCICR = (1 << PCIE1);
    sei();

    /* Wait while interrupt handler will catch R/W toggles */
    _delay_us(10);

    /* Disable interrupt */
    cli();
    PCMSK1 = 0;
    PCICR = 0;

    return rw_transition_count > 10;
}

void register_set_noop(uint8_t a, uint8_t b)
{
    return;
}

void reset_pc_noop(void)
{
    return;
}

void disable_interrupts_noop(void)
{
    return;
}

/*
  Initializes the interface to communicate with 6502
*/
void io_setup(void)
{
    // Configure the /RES pin on PORT C as output and set /RES high, also set
    // bits 0, 1 as output
    PORTC |= RES | RW;
    DDRC |= RES;

    bus_select(CPU_ADDRESS);

    bus_write(STA_abs);

    bus_deselect();

    // Set /RES low
    PORTC &= ~RES;

    // Hold it low for some time, for 6502 to perform reset
    _delay_us(10);

    // Set /RES high again
    PORTC |= RES;

    // Wait a while until the 6502 has completed its reset cycle
    _delay_us(100);

    /* Start out assuming no 2A03 is installed */
    io_clockdiv = 0;

    /* Check if 2A03 is installed and is running */
    if (detect_2a03_presence()) {
        /* Run detect_2a03_type to determine what kind of 2A03 is installed,
           this is used later to set up functions to talk to the 2A03. */
        uint8_t tries = 3;
        while (tries-- > 0) {
            io_clockdiv = detect_2a03_type();
            if (io_clockdiv == 12 || io_clockdiv == 15 || io_clockdiv == 16)
                break;
        }
    }

    /* Point function pointers to correct function based on which 2A03 model is
       being used */

    switch (io_clockdiv) {
    case 12:
        register_set = &register_set12;
        reset_pc = &reset_pc12;
        disable_interrupts = &disable_interrupts12;
        break;

    case 15:
        register_set = &register_set15;
        reset_pc = &reset_pc15;
        disable_interrupts = &disable_interrupts15;
        break;

    case 16:
        register_set = &register_set16;
        reset_pc = &reset_pc16;
        disable_interrupts = &disable_interrupts16;
        break;

        /* Fallback case when 2A03 is not present or not able to detect */
    default:
        register_set = register_set_noop;
        reset_pc = reset_pc_noop;
        disable_interrupts = disable_interrupts_noop;
        break;
    }

    // Disable interrupts on the 6502
    disable_interrupts();

    // Reset PC to ensure that it doesn't read any of the APU registers
    io_reset_pc();
}
