#pragma once

#include <avr/io.h>

#define F_CPU 20000000L
#define F_INTERRUPT 16000
#define F_DMC 16000
#define F_ENVELOPE 400
#define F_LFO 8000
#define BPM 100

uint16_t basspat[16];
uint16_t sq1_pat[16];
uint16_t sq2_pat[16];
uint8_t sq_mask[16];
