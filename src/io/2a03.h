/*
  Copyright 2014-2015 Johan Fjeldtvedt 

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



  io/2a03_io.h - Low level 2A03 I/O interface

  Contains functions for communicating with and controlling the 
  2A03.
*/


#pragma once

#include <stdint.h>

// The 6502 opcodes needed
#define LDA_imm 0xA9
#define STA_abs 0x8D
#define STA_zp 0x85
#define JMP_abs 0x4C

// Pins used to interface with 6502
#define RES 100             // PC2
#define RW 0b1000          // PC3

#define nop() asm volatile("nop")

void io_register_write(uint8_t reg, uint8_t value);
void io_write_changed(uint8_t reg);
void io_setup(void);
void io_reset_pc(void);

extern uint8_t io_reg_buffer[0x16];
extern uint8_t io_clockdiv;
