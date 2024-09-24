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



  SRAM interface with context switching

  Handles the low level details of reading and writing to/from the SRAM memory.
  Provides means of random access using a given adress, as well as writing or
  reading sequentially to/from memory.
*/


#pragma once

#include <avr/io.h>

#define WE 0b10000

#define we_low() PORTC &= ~WE
#define we_high() PORTC |= WE

#define MEMORY_SIZE 0x100000UL  // 1MB of memory

/*
   The memory context is needed to perform sequential operations while the
   memory is shared by several tasks.
*/
struct memory_context {
  uint8_t low;
  uint8_t mid;
  uint8_t high;
};

void memory_set_address(struct memory_context *context, uint32_t address);

void memory_write(uint32_t address, uint8_t value);
void memory_write_word(uint32_t address, uint16_t value);
void memory_write_dword(uint32_t address, uint32_t value);

uint8_t memory_read(uint32_t address);
uint16_t memory_read_word(uint32_t address);
uint32_t memory_read_dword(uint32_t address);
uint8_t memory_read_sequential(struct memory_context *context);
void memory_write_sequential(struct memory_context *context, uint8_t value);

void memory_setup(void);
void memory_clean(void);

uint32_t reverse_dword(uint32_t value);
