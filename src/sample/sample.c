/*
  Copyright 2014-2016 Johan Fjeldtvedt

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



  Sample system

  Contains functions for writing and reading samples to and from SRAM memory.
*/


#include "sample.h"
#include "io/memory.h"
#include <stdint.h>

#define NUM_SAMPLES 100

// 256b empty + 6400b patches + 16000b patterns
#define INDEX_START 22656

#define INDEX_ENTRY_SIZE 8
#define INDEX_SIZE (INDEX_ENTRY_SIZE * NUM_SAMPLES)

#define BLOCKTABLE_START (INDEX_START + INDEX_SIZE)
#define BLOCK_SIZE 1024
#define BLOCKTABLE_SIZE 1024
#define BLOCK_START BLOCKTABLE_START + BLOCKTABLE_SIZE

#define NUM_BLOCKS (MEMORY_SIZE - BLOCK_START) / BLOCK_SIZE

/* Internal functions */

static inline uint16_t get_next_block(uint16_t block);
static inline void link_blocks(uint16_t block, uint16_t next_block);
static inline void write_to_block(struct memory_context *mem_ctx, uint16_t block, uint16_t pos, uint8_t value);
static inline uint8_t read_from_block(struct memory_context *mem_ctx, uint16_t block, uint16_t pos);
static uint16_t allocate_block(void);
static inline void free_block(uint16_t block);
static inline uint32_t index_address(uint8_t index);
static void write_to_index(struct sample *sample, uint8_t index);
static void read_from_index(struct sample *sample, uint8_t index);
static void remove_from_index(uint8_t index);
static inline uint8_t index_occupied(uint8_t index);

/* Public */

void sample_clean(void)
/* Writes the sample index at the start of the sample area */
{
    for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
        if (index_occupied(i))
            sample_delete(i);
    }
}

void sample_reset(struct sample *sample)
{
    // Reset counters
    sample->current_position = 0;
    sample->current_block = sample->first_block;
    sample->bytes_done = 0;
}

void sample_load(struct sample *sample, uint8_t index)
{
    // Read sample data from index table
    read_from_index(sample, index);

    sample_reset(sample);
}

uint8_t sample_read_byte(struct sample *sample)
{
    uint8_t value = read_from_block(&sample->mem_ctx, sample->current_block, sample->current_position);

    if (++sample->current_position == BLOCK_SIZE) {
        sample->current_position = 0;
        sample->current_block = get_next_block(sample->current_block);
    }

    sample->bytes_done++;

    return value;
}

void sample_new(struct sample *sample, uint8_t index)
{
    if (index_occupied(index))
        sample_delete(index);

    sample->first_block = allocate_block();
    sample_reset(sample);

    write_to_index(sample, index);
}

void sample_write_serial(struct sample *sample, uint8_t value)
{
    write_to_block(&sample->mem_ctx, sample->current_block, sample->current_position, value);

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
    struct sample sample;
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

static inline void write_to_block(struct memory_context *mem_ctx, uint16_t block, uint16_t pos, uint8_t value)
{
    if (pos == 0)
        memory_set_address(mem_ctx, BLOCK_START + (uint32_t)block * BLOCK_SIZE);
    memory_write_sequential(mem_ctx, value);
}

static inline uint8_t read_from_block(struct memory_context *mem_ctx, uint16_t block, uint16_t pos)
{
    // Need to update the address only when starting on a new block:
    if (pos == 0)
        memory_set_address(mem_ctx, BLOCK_START + (uint32_t)block * BLOCK_SIZE);

    // Otherwise, just read sequentially from the block:
    return memory_read_sequential(mem_ctx);
}

static uint16_t allocate_block(void)
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

static void write_to_index(struct sample *sample, uint8_t index)
{
    uint32_t address = index_address(index);

    // Mark index as occupied
    memory_write(address++, 1);

    memory_write(address++, sample->type);

    memory_write_dword(address, sample->size);

    address += 4;

    memory_write_word(address, sample->first_block);
}

static void read_from_index(struct sample *sample, uint8_t index)
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
