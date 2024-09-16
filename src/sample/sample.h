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



  Sample system

  Contains functions for writing and reading samples to and from SRAM memory.
*/


#pragma once

#include <stdint.h>
#include "io/memory.h"

#define SAMPLE_TYPE_RAW 0
#define SAMPLE_TYPE_DPCM 1

#define SAMPLE_MIDI_LOW_INDEX 36

struct sample {
  uint8_t type;
  uint32_t size;

  // Internal
  uint16_t current_block;
  uint16_t current_position;
  uint32_t bytes_done;
  uint16_t first_block;

  struct memory_context mem_ctx;
};

void sample_clear_all(void);
void sample_new(struct sample *sample, uint8_t index);
void sample_load(struct sample *sample, uint8_t index);
void sample_reset(struct sample *sample);
uint8_t sample_read_byte(struct sample *sample);
void sample_delete(uint8_t index);
uint8_t sample_occupied(uint8_t index);
void sample_write_serial(struct sample *sample, uint8_t value);
