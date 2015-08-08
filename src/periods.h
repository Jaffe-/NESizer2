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

extern uint8_t note_min;
extern const uint8_t note_max;

uint16_t get_period(uint8_t chn, uint16_t c);
void periods_setup(void);
