#include "envelope.h"

Envelope env1;
Envelope env2;
Envelope env3;

void envelope_update(Envelope* env)
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
