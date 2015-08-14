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



  LFO implementation

  An 8-bit LFO with selectable wave shapes and variable frequency.
*/


#include "lfo/lfo.h"
#include <avr/pgmspace.h>
#include <stdint.h>
#include "apu/apu.h" 
#include "data/sine.c"

struct lfo lfo1;
struct lfo lfo2;
struct lfo lfo3;

void lfo_update(struct lfo *lfo)
{
  if (++lfo->counter == lfo->period) {
    lfo->counter = 0;

    switch (lfo->waveform) {
    case SINE:
      lfo->value = pgm_read_byte_near(&sine_table[lfo->position]);
      break;
      
    case RAMP_DOWN:
      lfo->value = 4 * lfo->position - 0x80;
      break;

    case RAMP_UP:
      lfo->value = 0x80 - 4 * lfo->position;
      break;

    case SQUARE:
      if (lfo->position < 32)
	lfo->value = -127;
      else
	lfo->value = 127;
      break;

    case TRIANGLE:
      if (lfo->position < 32) 
	lfo->value = 0x80 - 8 * lfo->position;
      else
	lfo->value = 8 * (lfo->position - 32) - 0x80;
      break;

    default:
      break;
    }

    if (++lfo->position == 64)
      lfo->position = 0;
  }
}

int16_t lfo_value(struct lfo *lfo, uint8_t intensity)
{
  if (intensity == 0) 
    return 0;
  else
    return lfo->value;
}

void lfo_update_handler(void)
{  
  lfo_update(&lfo1);
  lfo_update(&lfo2);
  lfo_update(&lfo3);
}

