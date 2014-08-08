#pragma once

// The 6502 opcodes needed
#define LDA_imm 0xA9
#define STA_abs 0x8D
#define LDA_abs 0xAD
#define STA_zp 0x85
#define JMP_abs 0x4C

// Pins used to interface with 6502
#define RES 100             // PC2
#define RW 0b1000          // PC3

#define nop() asm volatile("nop")

void io_register_write(uint8_t reg, uint8_t value);
void io_write_changed(uint8_t reg);
void io_setup();

uint8_t io_reg_buffer[0x16];

