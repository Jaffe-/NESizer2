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



  envelope.c - ADSR envelope implementation

  4-bit ADSR envelopes with 8-bit A, D, R settings.
*/


#include "envelope/envelope.h"

struct envelope env1;
struct envelope env2;
struct envelope env3;

void envelope_update(struct envelope* env)
/*
  Computes the next step in the given envelope. 
*/

{
  if (env->gate != env->gate_prev) {
    // If gate goes on, start the envelope
    if (env->gate_prev == 0) { 
      env->state = ATTACK;
      if (env->retrigger) 
	env->value = 0;
    }
		
    // Otherwise, start release phase
    else 
      env->state = RELEASE;

    env->counter = 0;
    env->gate_prev = env->gate;
  }

  // If there is no attack or decay, just set levels manually
  if (env->state == ATTACK && env->attack == 0) {
    env->state = DECAY;
    env->value = 15;
  }
  if (env->state == DECAY && env->decay == 0) {
    env->state = SUSTAIN;
    env->value = env->sustain;
  }

  if (env->state == SUSTAIN) 
    return;

  if (env->state == RELEASE && env->release == 0) 
    env->value = 0;

  uint8_t duration = 0;

  // Find out how long to count each step for to get desired envelope time:
  switch (env->state) {
  case ATTACK:
    duration = 2*env->attack;
    break;
  case DECAY:
    duration = 2*(15 * env->decay) / (15 - env->sustain);
    break;
  case RELEASE:
    duration = 2*(15 * env->release) / env->sustain;
    break;
  default:
    break;
  }

  // Increment the counter and return if not ready to update value yet
  env->counter++;
  if (env->counter == duration) {
    env->counter = 0;
    switch (env->state) {
    case ATTACK:
      if (env->value == 15) 
	env->state = DECAY;
      else
	env->value++;
      break;

    case DECAY:
      if (env->value == env->sustain)  
	env->state = SUSTAIN;
      else
	env->value--;

      break;

    case RELEASE:
      if (env->value == 0) 
	env->state = OFF;
      else
	env->value--;
      break;
      
    default:
      break;
    }
  }
  else return;
}

void envelope_update_handler()
{
  envelope_update(&env1);
  envelope_update(&env2);
  envelope_update(&env3);
}
