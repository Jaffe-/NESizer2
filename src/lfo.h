#pragma once

#include "apu.h"

typedef struct {
  int8_t period;
  enum {
    SINE = 1, RAMP_DOWN, RAMP_UP, SQUARE, TRIANGLE
  } waveform;

  // Used by LFO logic:
  int8_t value;
  uint8_t counter;
  uint8_t position;
} LFO;

extern LFO lfo1;
extern LFO lfo2;
extern LFO lfo3;

void lfo_update(LFO* lfo);
int16_t lfo_value(LFO* lfo, uint8_t intensity);
void lfo_update_handler();
