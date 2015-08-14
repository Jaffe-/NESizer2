/*
  Copyright 2014-2015 Johan Fjeldtvedt 

  This file is part of NESIZER.

  NESIZER is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  NESIZER is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NESIZER.  If not, see <http://www.gnu.org/licenses/>.



  lfo/lfo.h - LFO implementation

  An 8-bit LFO with selectable wave shapes and variable frequency.
*/


#pragma once

#include "apu/apu.h"

struct lfo {
  int8_t period;
  enum {
    SINE = 1, RAMP_DOWN, RAMP_UP, SQUARE, TRIANGLE
  } waveform;

  // Used by LFO logic:
  int8_t value;
  uint8_t counter;
  uint8_t position;
};

extern struct lfo lfo1;
extern struct lfo lfo2;
extern struct lfo lfo3;

void lfo_update(struct lfo* lfo);
int16_t lfo_value(struct lfo* lfo, uint8_t intensity);
void lfo_update_handler(void);
