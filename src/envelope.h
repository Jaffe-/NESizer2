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



  envelope.h - ADSR envelope implementation

  4-bit ADSR envelopes with 8-bit A, D, R settings.
*/


#pragma once

#include <avr/io.h>

enum env_state {ATTACK, DECAY, SUSTAIN, RELEASE, OFF};

struct envelope {
/* Envelope settings:
   
   attack, decay, release times
   sustain level 
   gate state (on or off)
   retrigger flag (retrigger the envelope when restarted)
*/

  int8_t attack;
  int8_t decay;
  int8_t sustain;
  int8_t release;

  uint8_t gate;
  uint8_t retrigger;

  // The following are internal:
  uint8_t value;
  enum env_state state;
  uint8_t counter;
  uint8_t gate_prev;
};

void envelope_update(struct envelope *env);
void envelope_update_handler(void);

extern struct envelope env1;
extern struct envelope env2;
extern struct envelope env3;
