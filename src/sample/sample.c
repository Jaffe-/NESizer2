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
#define BLOCKTABLE_SIZE 1024*2
#define BLOCK_START BLOCKTABLE_START + BLOCKTABLE_SIZE

#define NUM_BLOCKS (MEMORY_SIZE - BLOCK_START) / BLOCK_SIZE

/* Internal functions */

static inline uint16_t get_next_block(uint16_t block);
static uint16_t next_block_index(uint16_t block_entry);
static inline void link_blocks(uint16_t block_index, uint16_t next_block_index);
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

void sample_clear_all(void)
{
    for (uint32_t i = 0; i < INDEX_ENTRY_SIZE * NUM_SAMPLES; i++) {
        memory_write(INDEX_START + i, 0);
    }
    for (uint32_t i = 0; i < BLOCKTABLE_SIZE; i++) {
        memory_write(BLOCKTABLE_START + i, 0);
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
        sample->current_block = get_next_block(next_block_index(sample->current_block));
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

static uint16_t end_of_chain(uint16_t block_entry)
{
    return block_entry & 0x8000;
}

static uint16_t next_block_index(uint16_t block_entry)
{
    return block_entry & 0x3ff;
}

static uint16_t read_block_entry(uint16_t block_index)
{
    return memory_read_word(BLOCKTABLE_START + 2 * block_index);
}

void sample_delete(uint8_t index)
{
    struct sample sample;
    read_from_index(&sample, index);

    uint16_t block_index = sample.first_block;
    uint16_t block_entry;

    do {
        block_entry = read_block_entry(block_index);
        free_block(block_index);
        block_index = next_block_index(block_entry);
    } while (!end_of_chain(block_entry));

    remove_from_index(index);
}

uint8_t sample_occupied(uint8_t index)
{
    return index_occupied(index);
}


/* Internal function definitions */

static inline uint16_t get_next_block(uint16_t block_index)
{
    return memory_read_word(BLOCKTABLE_START + block_index * 2);
}

static inline void link_blocks(uint16_t block_index, uint16_t next_block_index)
/* Write the next block number at the block's location in the block table */
{
    /* Note: this keeps the block tree state, but removes the end of chain flag */
    uint16_t block_state = memory_read(BLOCKTABLE_START + block_index * 2 + 1) & 0x3c;
    uint16_t block_entry = (block_state << 8) | next_block_index;
    memory_write_word(BLOCKTABLE_START + block_index * 2, block_entry);
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

/*
  Tree based block allocation

  Each block table entry has the following info:

  +---+--------------+------------+
  | T | child-states | next-block |
  +---+--------------+------------+
   15   13..10        9..0

   The block table holds the state of two data structures for
   each entry.
   One is a FAT like linked list:
      T            - chain terminated flag
      next-block   - next block in the chain

   The second is a tree used for fast allocation. This information
   is not related to the block itself, but it is co-located with
   block data to make address calculations quick.

      child-states - the occupancy state of each child node, if
                     a bit is set here it means the corresponding
                     child has at least one free block

*/

static inline uint8_t get_child_states(uint8_t block_entry_upper)
{
    return (block_entry_upper >> 2) & 0x0f;
}

static inline uint8_t set_child_states(uint8_t block_entry_upper, uint8_t val)
{
    return (block_entry_upper & 0x83) | (val << 2);
}

static inline uint8_t get_next_child(uint8_t child_states)
{
    for (uint8_t i = 0; i < 4; i++) {
        if (!(child_states & (1 << i)))
            return i;
    }
    return 0xff;
}

static inline uint16_t get_block_index(uint16_t logical_block_index, uint8_t level)
{
    if (level == 0)
        return 1023;
    return logical_block_index + (4 - level);
}

static uint16_t allocate_block(void)
{
    uint8_t path_cache[5];
    uint16_t logical_block_index = 0;
    uint8_t next_child = 0;
    uint8_t block_entry_upper;

    for (uint8_t level = 0; level < 5; level++) {
        logical_block_index |= next_child;
        logical_block_index <<= 2;

        uint16_t block_index = get_block_index(logical_block_index, level);
        block_entry_upper = memory_read(BLOCKTABLE_START + 2 * block_index + 1);
        uint8_t child_states = get_child_states(block_entry_upper);
        next_child = get_next_child(child_states);
        path_cache[level] = block_entry_upper;
    }

    uint16_t new_block_index = logical_block_index | next_child;

    // Mark block as end of chain
    block_entry_upper = memory_read(BLOCKTABLE_START + 2 * new_block_index + 1) | 0x80;
    memory_write(BLOCKTABLE_START + 2 * new_block_index + 1, block_entry_upper);

    // Reverse through cached blocks and update them
    logical_block_index = new_block_index;

    for (int8_t level = 4; level >= 0; level--) {
        uint8_t child_index = logical_block_index & 0x03;
        logical_block_index &= ~0x03;
        uint16_t block_index = get_block_index(logical_block_index, level);
        logical_block_index >>= 2;

        block_entry_upper = path_cache[level];

        /* The block we are updating from cache might be the one we just allocated,
           if so, we need to also update it with the end-of-chain marker */
        if (block_index == new_block_index)
            block_entry_upper |= 0x80;

        uint8_t child_states = get_child_states(block_entry_upper);
        child_states |= (1 << child_index);
        memory_write(BLOCKTABLE_START + 2 * block_index + 1,
                     set_child_states(block_entry_upper, child_states));

        /* This node still has free blocks after marking the child as
           occupied, so we are done updating the path from the root with
           new child states */
        if (child_states != 0x0f)
            break;
    }

    return new_block_index;
}

static void free_block(uint16_t block_index)
{
    uint16_t logical_block_index = block_index;

    for (int8_t level = 4; level >= 0; level--) {
        uint8_t child_index = logical_block_index & 0x03;
        logical_block_index &= ~0x03;
        block_index = get_block_index(logical_block_index, level);
        logical_block_index >>= 2;
        uint8_t block_entry_upper = memory_read(BLOCKTABLE_START + 2 * block_index + 1);
        uint8_t child_states = get_child_states(block_entry_upper);

        /* This child is already marked as having free space,
           so we can stop updating the path back to the root. */
        if (!(child_states & (1 << child_index)))
            return;

        child_states &= ~(1 << child_index);
        memory_write(BLOCKTABLE_START + 2 * block_index + 1,
                     set_child_states(block_entry_upper, child_states));
    }
}


/*
  Index table

  The index table keeps track of 100 sample 'files',
  containing an occupation flag, type, sample size and a pointer
  to the first block of data in the sample.
*/
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
