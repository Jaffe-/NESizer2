/*
  Copyright 2014-2016 Johan Fjeldtvedt

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

struct lfo lfo[3];

void lfo_update(struct lfo *lfo)
{
    if (++lfo->counter == lfo->period) {
        lfo->counter = 0;

        switch (lfo->waveform) {
        case SINE:
            lfo->value = pgm_read_byte_near(&sine_table[lfo->position]);
            break;

        case RAMP_DOWN:
            lfo->value = 4*(lfo->position - 32);
            break;

        case RAMP_UP:
            lfo->value = 4*(32 - lfo->position);
            break;

        case SQUARE:
            if (lfo->position < 32)
                lfo->value = -127;
            else
                lfo->value = 127;
            break;

        case TRIANGLE:
            if (lfo->position < 32)
                lfo->value = 8 * (lfo->position - 16);
            else if (lfo->position == 32)
                lfo->value = 127;
            else
                lfo->value = 8 * (48 - lfo->position);
            break;

        default:
            break;
        }

        if (++lfo->position == 64)
            lfo->position = 0;
    }
}

void lfo_update_handler(void)
{
    lfo_update(&lfo[0]);
    lfo_update(&lfo[1]);
    lfo_update(&lfo[2]);
}
