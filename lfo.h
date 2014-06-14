#pragma once

#include "apu.h"

#define LFO_TRIANGLE 0
#define LFO_SQUARE 1
#define LFO_SINE 2
#define LFO_RAMP_UP 3
#define LFO_RAMP_DOWN 4

typedef struct {
    uint8_t period;
    uint8_t waveform : 3;

    // Used by LFO logic:
    int8_t value;
    uint8_t counter;
    uint8_t position;
} LFO;

LFO lfo1;
LFO lfo2;
LFO lfo3;

void lfo_update(LFO* lfo);
void lfo_apply_square(LFO*, Square*, uint8_t);
void lfo_apply_triangle(LFO*, Triangle*, uint8_t);
