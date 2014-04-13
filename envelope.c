#include "envelope.h"

void envelope_update(Envelope* env)
/*
  Computes the next step in the given envelope.
  Stops the envelope if gate switches off and starts 
  it switches on. The function expects to be called at
  a 250 Hz frequency (f_cpu / (156 * 64 * 32)).
*/

{
    if (env->gate != env->gate_prev) {
	// If gate goes on, start the envelope
	if (env->gate_prev == 0) 
	    env->phase = ATTACK;
		
	// Otherwise, start release phase
	else 
	    env->phase = RELEASE;

	env->counter = 0;
	env->gate_prev = env->gate;
    }

    // If there is no attack or decay, just set levels manually
    if (env->phase == ATTACK && env->attack == 0) {
	env->phase = DECAY;
	env->value = 15;
    }
    if (env->phase == DECAY && env->decay == 0) {
	env->phase = SUSTAIN;
	env->value = env->sustain;
    }

    uint8_t duration;

    // Find out how long to count each step for to get desired envelope time:
    switch (env->phase) {
    case ATTACK:
	duration = 4*env->attack;
	break;
    case DECAY:
	duration = 4*(15 * env->decay) / (15 - env->sustain);
	break;
    case RELEASE:
	duration = 4*(15 * env->release) / env->sustain;
	break;
    }

    // Increment the counter and return if not ready to update value yet
    env->counter++;
    if (env->counter == duration) {
	env->counter = 0;
	switch (env->phase) {
	case ATTACK:
	    env->value++;
	    if (env->value == 15) 
		env->phase = DECAY;
	    break;

	case DECAY:
	    env->value--;
	    if (env->value == env->sustain)  
		env->phase = SUSTAIN;
	    break;

	case RELEASE:
	    if (env->value == 0) 
		env->phase = OFF;
	    else
		env->value--;
	    break;
	}
    }
    else return;
    
}

