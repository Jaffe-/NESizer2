/*
  NESIZER

  Sample system

  (c) Johan Fjeldtvedt

  Contains functions for writing and reading samples to and from SRAM memory. 
*/

#include "sample.h"
#include "memory.h"
#include <stdint.h>

#define NUM_SAMPLES 100

#define INDEX_START 0x6000
#define BLOCKTABLE_START 0x6400
#define BLOCK_START 0x6C00UL

#define INDEX_ENTRY_SIZE 8
#define INDEX_SIZE (INDEX_ENTRY_SIZE * NUM_SAMPLES)

#define BLOCK_SIZE 1024
#define BLOCKTABLE_SIZE 2048

#define NUM_BLOCKS (MEMORY_SIZE - BLOCK_START) / BLOCK_SIZE


/* Internal functions */

static inline uint16_t get_next_block(uint16_t block);
static inline void link_blocks(uint16_t block, uint16_t next_block);
static inline void write_to_block(uint16_t block, uint16_t pos, uint8_t value);
static inline uint8_t read_from_block(uint16_t block, uint16_t pos);
static uint16_t allocate_block();
static inline void free_block(uint16_t block);
static inline uint32_t index_address(uint8_t index);
static void write_to_index(Sample* sample, uint8_t index);
static void read_from_index(Sample* sample, uint8_t index);
static void remove_from_index(uint8_t index);
static inline uint8_t index_occupied(uint8_t index);

/* Public functions */

void sample_clean()
/* Writes the sample index at the start of the sample area */
{
  for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
    if (index_occupied(i))
      sample_delete(i);
  }
}

void sample_reset(Sample* sample)
{
  // Reset counters
  sample->current_position = 0;
  sample->current_block = sample->first_block;
  sample->bytes_done = 0;
}

void sample_load(Sample* sample, uint8_t index)
{
  // Read sample data from index table
  read_from_index(sample, index);

  sample_reset(sample);
}

uint8_t sample_read_byte(Sample* sample)
{
  uint8_t value = read_from_block(sample->current_block, sample->current_position);

  if (++sample->current_position == BLOCK_SIZE) {
    sample->current_position = 0;
    sample->current_block = get_next_block(sample->current_block);
  }

  sample->bytes_done++;

  return value;
}

void sample_new(Sample* sample, uint8_t index)
{
  sample->first_block = allocate_block();
  sample_reset(sample);

  if (index_occupied(index)) 
    sample_delete(index);

  write_to_index(sample, index);
}

void sample_write_serial(Sample* sample, uint8_t value) 
{
  write_to_block(sample->current_block, sample->current_position, value);
    
  sample->bytes_done++;

  if (++sample->current_position == BLOCK_SIZE) {
    sample->current_position = 0;
    uint16_t new_block = allocate_block();
    link_blocks(sample->current_block, new_block);
    sample->current_block = new_block;
  }
}

void sample_delete(uint8_t index)
{
  Sample sample;
  read_from_index(&sample, index);
    
  uint16_t block = sample.first_block;
    
  for (uint16_t i = 0; i < sample.size / BLOCK_SIZE; i++) {
    uint16_t next_block = get_next_block(block);
    free_block(block);
    block = next_block;
  }

  remove_from_index(index);
}

uint8_t sample_occupied(uint8_t index)
{
  return index_occupied(index);
}


/* Internal function definitions */

static inline uint16_t get_next_block(uint16_t block)
{
  // Find the next block from the block table
  return memory_read_word(BLOCKTABLE_START + block * 2) - 1;
}

static inline void link_blocks(uint16_t block, uint16_t next_block)
/* Write the next block number at the block's location in the block table */
{
  memory_write_word(BLOCKTABLE_START + block * 2, next_block + 1); 
}

static inline void write_to_block(uint16_t block, uint16_t pos, uint8_t value)
{
  if (pos == 0)
    memory_set_address(BLOCK_START + (uint32_t)block * BLOCK_SIZE);
  memory_write_sequential(value);
}

static inline uint8_t read_from_block(uint16_t block, uint16_t pos)
{
  // Need to update the address only when starting on a new block:
  if (pos == 0)
    memory_set_address(BLOCK_START + (uint32_t)block * BLOCK_SIZE);
	
  // Otherwise, just read sequentially from the block:
  return memory_read_sequential();
}

static uint16_t allocate_block()
{
  for (uint16_t i = 0; i < NUM_BLOCKS; i++) {
    uint16_t block_entry = memory_read_word(BLOCKTABLE_START + i * 2); 
    if (block_entry == 0) {
      // Mark the block as in use 
      memory_write_word(BLOCKTABLE_START + i * 2, 0xFFFF);

      // Return allocated block number
      return i;
    }
  }	
    
  // Return error code if no free block was found
  return 0xFFFF;
}

static inline void free_block(uint16_t block)
{
  // Write 0 to block table to indicate that the block is now free
  memory_write_word(BLOCKTABLE_START + block * 2, 0);
}

static inline uint32_t index_address(uint8_t index)
{
  return INDEX_START + (uint16_t)index * INDEX_ENTRY_SIZE;
}

static void write_to_index(Sample* sample, uint8_t index)
{
  uint32_t address = index_address(index);

  // Mark index as occupied
  memory_write(address++, 1);

  memory_write(address++, sample->type);

  memory_write_dword(address, sample->size);
    
  address += 4;
  memory_write_word(address, sample->first_block);
}

static void read_from_index(Sample* sample, uint8_t index)
{
  uint32_t address = index_address(index) + 1;
   
  sample->type = memory_read(address++);

  sample->size = memory_read_dword(address);
    
  address += 4;
  sample->first_block = memory_read_word(address);
}

static void remove_from_index(uint8_t index)
{
  memory_write(index_address(index), 0);
}

static inline uint8_t index_occupied(uint8_t index)
{
  return memory_read(index_address(index));
}

