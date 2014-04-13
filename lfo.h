#pragma once

#include "apu.h"

#define LFO_TRIANGLE 0
#define LFO_SQUARE 1
#define LFO_SINE 2
#define LFO_RAMP_UP 3
#define LFO_RAMP_DOWN 4

typedef struct {
    uint8_t base_p;
    uint8_t period;
    uint8_t waveform : 3;
    uint8_t intensity;
    int16_t output;

    // Used by LFO logic:
    int8_t value;
    uint8_t counter;
    uint8_t position;
} LFO;

LFO lfo1;
LFO lfo2;
LFO lfo3;

void lfo_update(LFO* lfo);
void lfo_apply_square(Square*, LFO*);
void lfo_apply_triangle(Triangle*, LFO*);
