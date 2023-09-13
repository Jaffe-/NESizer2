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


#include "memory.h"
#include <avr/io.h>
#include "bus.h"

// Useful unions for accessing individual bytes
union val32 {
  uint8_t bytes[4];
  uint32_t value;
};

union val16 {
  uint8_t bytes[2];
  uint16_t value;
};

struct memory_context default_context;
struct memory_context *current_context;

// Functions for writing to each of the three address latches

static inline void set_addrlow(uint8_t addrlow)
{
  bus_select(MEMORY_LOW_ADDRESS);
  bus_write(addrlow);
}

static inline void set_addrmid(uint8_t addrmid)
{
  bus_select(MEMORY_MID_ADDRESS);
  bus_write(addrmid);
}

static inline void set_addrhigh(uint8_t addrhigh)
{
  bus_select(MEMORY_HIGH_ADDRESS);

  /* In the case of the high address, the value written to this latch also
     controls the Chip Enable (CE) signals of the SRAMs. The lower three bits
     of the latch are connected to the upper three bits of each SRAM's address 
     inputs, while the fourth bit is the first SRAM's CE signal, and the fifth
     is the second SRAM's CE signal. The latter two must be set to zero
     depending on whether the most significant bit of addrhigh is set, in order
     to select the correct SRAM IC.*/ 
  bus_write((addrhigh & 0x07) | ((addrhigh & 0x08) ? 0b01000 : 0b10000));
}

static void inc_address(void)
{
  set_addrlow(++current_context->low);
  if (current_context->low == 0) {
    // Current high is increased, but not written to the latch until
    // the SRAM is about to be used (the chip select bits have to be set).
    if (++current_context->mid == 0)
      current_context->high++;

    set_addrmid(current_context->mid);
  }

  bus_deselect();
}

static inline void deselect(void)
/*
  Sets both chip select bits to one in the high latch.
*/
{
  bus_select(MEMORY_HIGH_ADDRESS);
  bus_write(0b11000);

  bus_deselect();
}

static inline uint8_t read_data(void)
{
  set_addrhigh(current_context->high);

  bus_deselect();
  bus_dir_input();
    
  uint8_t value = bus_read();
    
  bus_dir_output();
    
  deselect();

  return value;
}

static inline void write_data(uint8_t value)
{
  set_addrhigh(current_context->high);
    
  // Switch to memory data address and put value on bus:
  bus_deselect();
  we_low();
  bus_write(value);
  we_high();

  deselect();
}

static inline uint8_t read_sequential(void)
{
  uint8_t value = read_data();

  inc_address();

  return value;
}

static inline void write_sequential(uint8_t value)
{
  write_data(value);
  inc_address();
}

static void apply_context(struct memory_context *context)
{
  set_addrlow(context->low);
  set_addrmid(context->mid);
  set_addrhigh(context->high);
  current_context = context;
}

static inline void check_context(struct memory_context *context)
{
  // If we changed context, the address latches need to be updated
  if (context != current_context)
    apply_context(context);
}

void memory_set_address(struct memory_context *context, uint32_t address)
/*
   Sets an adress. The latch values are saved in the context for
   sequential writing later.
*/
{
  current_context = context;
  
  union val32 addr = {.value = address};

  // Put first 8 address bits in low address latch:
  set_addrlow(context->low = addr.bytes[0]);

  // Put next 8 address bits in mid address latch:
  set_addrmid(context->mid = addr.bytes[1]);

  // Put next 4 address bits in high address latch.
  // The final bit decides between the first and second memory bank.
  // This needs to be translated to corresponding chip select signals.
  context->high = addr.bytes[2] & 0x0F;
}

void memory_write(uint32_t address, uint8_t value)
{
  memory_set_address(&default_context, address);

  write_data(value);
}

uint8_t memory_read(uint32_t address)
{    
  // Set address:
  memory_set_address(&default_context, address);

  uint8_t value = read_data();
	
  return value;
}

uint8_t memory_read_sequential(struct memory_context *context)
/*
  Reads a byte at the current address and increments address by 1.
*/
{
  check_context(context);
  return read_sequential();
}

void memory_write_sequential(struct memory_context *context, uint8_t value)
{
  check_context(context);
  write_sequential(value);
}

void memory_write_word(uint32_t address, uint16_t value)
{
  memory_set_address(&default_context, address);

  union val16 val = {.value = value};
  write_sequential(val.bytes[0]);
  write_sequential(val.bytes[1]);
}

uint16_t memory_read_word(uint32_t address)
{
  memory_set_address(&default_context, address);

  union val16 val;
  val.bytes[0] = read_sequential();
  val.bytes[1] = read_sequential();
  return val.value;
}

void memory_write_dword(uint32_t address, uint32_t value)
{
  memory_set_address(&default_context, address);

  union val32 val = {.value = value};
  write_sequential(val.bytes[0]);
  write_sequential(val.bytes[1]);
  write_sequential(val.bytes[2]);
  write_sequential(val.bytes[3]);
}

uint32_t memory_read_dword(uint32_t address)
{
  memory_set_address(&default_context, address);

  union val32 val;
  val.bytes[0] = read_sequential();
  val.bytes[1] = read_sequential();
  val.bytes[2] = read_sequential();
  val.bytes[3] = read_sequential();
  return val.value;
}

void memory_clean(void)
{
  memory_set_address(&default_context, 0);
  for (uint32_t i = 0; i < MEMORY_SIZE; i++) 
    memory_write_sequential(&default_context, 0);
}

void memory_setup(void)
{
  // Make sure the upper address latch is driving CE1 and CE2 high (not asserted)
  deselect();

  // Set WE high (not asserted)
  PORTC |= WE;
  DDRC |= WE;
}

