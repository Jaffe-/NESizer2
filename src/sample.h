#pragma once

#include <stdint.h>

#define SAMPLE_TYPE_RAW 0
#define SAMPLE_TYPE_DPCM 1

typedef struct {
  uint8_t type;
  uint32_t size;
  uint16_t first_block;
    
  // Internal
  uint16_t current_block;
  uint16_t current_position;
  uint32_t bytes_done;
} Sample;

extern Sample sample;

void sample_clean();
void sample_new(uint8_t index);
void sample_load(uint8_t index);
void sample_reset();
uint8_t sample_read_byte();
void sample_delete(uint8_t index);
uint8_t sample_occupied(uint8_t index);
void sample_write_serial(uint8_t value);
