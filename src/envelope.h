#pragma once

#include <avr/io.h>

typedef enum {ATTACK, DECAY, SUSTAIN, RELEASE, OFF} env_state_e;

typedef struct {
/* Envelope settings:
   
   attack, decay, release times
   sustain level 
   gate state (on or off)
   retrigger flag (retrigger the envelope when restarted)
*/

  uint8_t attack;
  uint8_t decay;
  uint8_t sustain;
  uint8_t release;
  uint8_t gate;
  uint8_t retrigger;

  // The following are internal:
  uint8_t value;
  env_state_e state;
  uint8_t counter;
  uint8_t gate_prev;
} Envelope;

void envelope_update(Envelope* env);
void envelope_update_handler();

extern Envelope env1;
extern Envelope env2;
extern Envelope env3;
