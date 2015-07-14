#pragma once

#include <avr/io.h>

#define WE 0b10000

#define we_low() PORTC &= ~WE
#define we_high() PORTC |= WE

#define MEMORY_SIZE 0x100000UL  // 1MB of memory

typedef union {
  uint8_t bytes[4];
  uint32_t value;
} val32_t;

typedef union {
  uint8_t bytes[2];
  uint16_t value;
} val16_t;

void memory_set_address(uint32_t address);

void memory_write(uint32_t address, uint8_t value);
void memory_write_word(uint32_t address, uint16_t value);
void memory_write_dword(uint32_t address, uint32_t value);

uint8_t memory_read(uint32_t address);
uint16_t memory_read_word(uint32_t address);
uint32_t memory_read_dword(uint32_t address);
uint8_t memory_read_sequential();
void memory_write_sequential(uint8_t value);

void memory_setup(void);
void memory_clean(void);
