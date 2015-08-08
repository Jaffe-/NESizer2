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



  Portamento implementation

  A simple portamento implementation. Computes delta values in cents
  to be applied by modulation.
*/


#include <stdint.h>
#include "portamento.h"
#include "modulation.h"

int8_t portamento_values[3] = {0};
uint16_t portamento_cs[3] = {0};
uint8_t portamento_target_notes[3] = {0};

static uint8_t counters[3] = {0};

static inline uint16_t note_to_c(uint8_t note)
{
  return note << 6;
}

void portamento_handler(void)
{
  for (uint8_t i = 0; i < 3; i++) {
    if (portamento_values[i] == 0)
      portamento_cs[i] = (uint16_t)portamento_target_notes[i] << 6;
    else {
      if (++counters[i] == portamento_values[i]) {
	counters[i] = 0;
	if (portamento_cs[i] > note_to_c(portamento_target_notes[i]))
	  portamento_cs[i]--;
	else if (portamento_cs[i] < note_to_c(portamento_target_notes[i]))
	  portamento_cs[i]++;
      }
    }
  }
}

