#pragma once

#define WE 0b10000
#define MEM_EN 0b10000000

#define we_low() PORTC &= ~WE
#define we_high() PORTC |= WE

#define MEMORY_SIZE 0x80000

void memory_set_address(uint32_t address);

void memory_write(uint32_t address, uint8_t value);
void memory_write_word(uint32_t address, uint16_t value);
void memory_write_dword(uint32_t address, uint32_t value);

uint8_t memory_read(uint32_t address);
uint16_t memory_read_word(uint32_t address);
uint32_t memory_read_dword(uint32_t address);
uint8_t memory_read_sequential();

void memory_setup();
void memory_clean();
