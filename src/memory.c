/* 
   NESIZER
   External SRAM memory routines

   (c) Johan Fjeldtvedt

   Handles the low level details of reading and writing to/from the SRAM memory.
   Provides means of random access using a given adress, as well as writing or 
   reading sequentially to/from memory. 
*/

#include "memory.h"
#include <avr/io.h>
#include "bus.h"

// Internal three byte representation of the current address. 
static uint8_t current_low;
static uint8_t current_mid;
static uint8_t current_high;

// Functions for writing to each of the three address latches

inline void set_addrlow(uint8_t addrlow)
{
  bus_select(MEMORY_LOW_ADDRESS);
  bus_write(addrlow);
}

inline void set_addrmid(uint8_t addrmid)
{
  bus_select(MEMORY_MID_ADDRESS);
  bus_write(addrmid);
}

inline void set_addrhigh(uint8_t addrhigh)
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

inline void deselect()
/*
  Sets both chip select bits to one in the high latch.
*/
{
  bus_select(MEMORY_HIGH_ADDRESS);
  bus_write(0b11000);

  bus_deselect();
}

static inline uint8_t read_data()
{
  set_addrhigh(current_high);

  bus_deselect();
  bus_dir_input();
    
  uint8_t value = bus_read();
    
  bus_dir_output();
    
  deselect();

  return value;
}

static inline void write_data(uint8_t value)
{
  set_addrhigh(current_high);
    
  // Switch to memory data address and put value on bus:
  bus_deselect();
  we_low();
  bus_write(value);
  we_high();

  deselect();
}

static void inc_address()
{
  set_addrlow(++current_low);
  if (current_low == 0) {
    // Current high is increased, but not written to the latch until
    // the SRAM is about to be used (the chip select bits have to be set).
    if (++current_mid == 0)
      current_high++;
	
    set_addrmid(current_mid);
  }

  bus_deselect();
}

void memory_set_address(uint32_t address)
/* 
   Sets an adress. The latch values are saved for
   sequential writing.
*/
{
  val32_t addr = {.value = address};
  
  // Put first 8 address bits in low address latch:
  set_addrlow(current_low = addr.bytes[0]);
    
  // Put next 8 address bits in mid address latch:
  set_addrmid(current_mid = addr.bytes[1]);

  // Put next 4 address bits in high address latch.
  // The final bit decides between the first and second memory bank.
  // This needs to be translated to corresponding chip select signals.
  current_high = addr.bytes[2] & 0x0F;
}

void memory_write(uint32_t address, uint8_t value)
{
  memory_set_address(address);

  write_data(value);
}

uint8_t memory_read(uint32_t address)
{    
  // Set address:
  memory_set_address(address);

  uint8_t value = read_data();
	
  return value;
}

uint8_t memory_read_sequential()
/*
  Reads a byte at the current address and increments address by 1.
*/
{
  // Set high byte. This has to be done even though the high byte 
  // might not have (and probably hasn't) changed, because the chip 
  // select bit has to be set. 

  uint8_t value = read_data();

  inc_address();

  return value;
}

void memory_write_sequential(uint8_t value)
{
  write_data(value);

  inc_address();
}

void memory_write_word(uint32_t address, uint16_t value)
{
  memory_set_address(address);

  val16_t val = {.value = value};
  memory_write_sequential(val.bytes[0]);
  memory_write_sequential(val.bytes[1]);
}

uint16_t memory_read_word(uint32_t address)
{
  memory_set_address(address);

  val16_t val;
  val.bytes[0] = memory_read_sequential();
  val.bytes[1] = memory_read_sequential();
  return val.value;
}

void memory_write_dword(uint32_t address, uint32_t value)
{
  memory_set_address(address);

  val32_t val = {.value = value};
  memory_write_sequential(val.bytes[0]);
  memory_write_sequential(val.bytes[1]);
  memory_write_sequential(val.bytes[2]);
  memory_write_sequential(val.bytes[3]);
}

uint32_t memory_read_dword(uint32_t address)
{
  memory_set_address(address);

  val32_t val;
  val.bytes[0] = memory_read_sequential();
  val.bytes[1] = memory_read_sequential();
  val.bytes[2] = memory_read_sequential();
  val.bytes[3] = memory_read_sequential();
  return val.value;
}

void memory_clean()
{
  memory_set_address(0);
  for (uint32_t i = 0; i < MEMORY_SIZE; i++) 
    memory_write_sequential(0);
}

void memory_setup()
{
  // Set WE (pin 5) as output
  DDRC |= WE;

  // Set WE high
  PORTC |= WE;

  // Make sure the upper address latch doesn't enable its outputs
  deselect();
}

