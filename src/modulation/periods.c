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
#ifdef TARGET
    #include "io/2a03.h"
#else
    #include "io_stubs/2a03.h"
#endif

/* 
   The tables were auto generated using

   T = f_2A03 / (16 * f) - 1
 
*/

const uint16_t period_table12[84] PROGMEM = {
  3185, 3006, 2837, 2678, 2527, 2385, 2252, 2125, 2006, 1893, 1787, 1686, 
  1592, 1502, 1418, 1338, 1263, 1192, 1125, 1062, 1002, 946, 893, 843, 
  795, 751, 708, 669, 631, 596, 562, 531, 501, 473, 446, 421, 
  397, 375, 354, 334, 315, 297, 281, 265, 250, 236, 222, 210, 
  198, 187, 176, 166, 157, 148, 140, 132, 124, 117, 111, 104, 
  99, 93, 88, 83, 78, 74, 69, 65, 62, 58, 55, 52, 
  49, 46, 43, 41, 39, 36, 34, 32, 30, 29, 27, 25
};

const uint16_t period_table15[84] PROGMEM = {
  2547, 2404, 2269, 2142, 2022, 1908, 1801, 1700, 1604, 1514, 1429, 1349, 
  1273, 1202, 1134, 1070, 1010, 954, 900, 849, 802, 757, 714, 674, 
  636, 600, 567, 535, 505, 476, 450, 424, 400, 378, 357, 336, 
  318, 300, 283, 267, 252, 238, 224, 212, 200, 188, 178, 168, 
  158, 149, 141, 133, 125, 118, 112, 105, 99, 94, 88, 83, 
  79, 74, 70, 66, 62, 59, 55, 52, 49, 46, 44, 41, 
  39, 37, 34, 32, 31, 29, 27, 26, 24, 23, 21, 20
};

const uint16_t period_table16[84] PROGMEM = {
  2388, 2254, 2127, 2008, 1895, 1789, 1688, 1594, 1504, 1420, 1340, 1265, 
  1194, 1127, 1063, 1004, 947, 894, 844, 796, 752, 709, 669, 632, 
  596, 563, 531, 501, 473, 446, 421, 398, 375, 354, 334, 315, 
  298, 281, 265, 250, 236, 223, 210, 198, 187, 177, 167, 157, 
  148, 140, 132, 125, 118, 111, 105, 99, 93, 88, 83, 78, 
  74, 69, 66, 62, 58, 55, 52, 49, 46, 43, 41, 39, 
  36, 34, 32, 30, 29, 27, 25, 24, 23, 21, 20, 19
};

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
    
  val = (1.0f - 0.00087696f * tone.offset) * base_period;  

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
