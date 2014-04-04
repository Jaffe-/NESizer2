#pragma once

// The 6502 opcodes needed
#define LDA_imm 0xA9
#define STA_abs 0x8D
#define LDA_abs 0xAD
#define STA_zp 0x85
#define JMP_abs 0x4C

// Pins used to interface with 6502
#define RES 1             // PC0
#define PHI2 0b10         // PC1
#define RW 0b100          // PC2

void register_write(uint8_t reg, uint8_t value);
void register_write_all();
uint8_t status_read();   
void setup_2a03();

uint8_t reg_buffer[0x16];
