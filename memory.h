#pragma once

#define WE 0b10000

#define we_low() PORTC &= ~WE
#define we_high() PORTC |= WE

void memory_write(uint32_t address, uint8_t value);
uint8_t memory_read(uint32_t address);
void memory_setup();
