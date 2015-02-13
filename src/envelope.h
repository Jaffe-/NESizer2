#pragma once

#include <avr/io.h>

// Envelope states
#define ATTACK 0
#define DECAY 1
#define SUSTAIN 2
#define RELEASE 3
#define OFF 4

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
    uint8_t gate : 1;
    uint8_t retrigger : 1;

    // The following are internal:
    uint8_t value : 4;
    uint8_t state : 3;
    uint8_t counter;
    uint8_t gate_prev : 1;
} Envelope;

void envelope_update(Envelope* env);
void envelope_update_handler();

Envelope env1;
Envelope env2;
Envelope env3;
