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



  Period tables

  Contains tables of periods to apply to a channel to get a desired
  note frequency. There are three different tables adjusted for each
  of the three supported 2A03 chips.
*/


#include <stdint.h>
#include <avr/pgmspace.h>
#include "modulation/periods.h"
#include "io/2a03.h"
#include "modulation/period_table.h"

const uint16_t *period_table;

uint8_t note_min;
const uint8_t note_max = 72;

union tone {
  uint16_t raw_value;
  struct { 
    uint8_t offset : 6;
    uint16_t semitone : 10;
  } __attribute__((packed));
};

uint16_t get_period(uint8_t chn, uint16_t c)
{
  /*
    Converts the given frequency change (given in steps of 64 cents) to a 
    corresponding timer value change.
    
    Lower 6 bits is steps between semitones
    Rest of bits are semitones, where 0 corresponds to C1 etc.
    
    DETAILS:

    By direct computation, 
    dT = f_2A03 / (16 * (f + df)) - f_2A03 / (16 * f)
    = f_2A03 / 16 * (f/(f(f + df)) - (f + df) / (f(f + df)))
    = f_2A03 / (16 f) * df / (f + df) 
    = T * (1 - f / (f + df))
    Further more, by the definition of cents and adjusting for dc's 10 cent scale:
    f + df = f * (2^(dc/120) 
    Putting these two together yields
    dT = T * (2^(-dc/120) - 1).

    To simplify calculating the power of 2, a piecewise linear approximation is
    used.
  */ 
  union tone tone;
  tone.raw_value = c;

  uint16_t val;
  uint8_t tri_scale = 0;
  uint16_t base_period;
  
  if (chn == 2 && tone.semitone < 12) {
    base_period = pgm_read_word_near(&period_table[tone.semitone + 12]);
  }
  else {
    base_period = pgm_read_word_near(&period_table[tone.semitone]);
    if (chn == 2)
      tri_scale = 1;
  }
    
  val = (1.0f - 0.00087696f * tone.offset) * base_period - 1;

  // Triangle channel timer value is exactly half of (T - 1).
  if (tri_scale) {
    val = (val - 1) >> 1;
  }
    
  // If value is out of bounds, discard the change
  if (val > 2047)
    return 2047;
  else if (val < 8)
    return 8;
  else
    return val;
}

void periods_setup(void)
{
  switch (io_clockdiv) {
  case 12:
    period_table = period_table12;
    break;
  case 15:
    period_table = period_table15;
    break;
  case 16:
    period_table = period_table16;
    break;
  }
}
