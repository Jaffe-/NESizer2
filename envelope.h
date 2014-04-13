#pragma once

#include <avr/io.h>

#define ATTACK 0
#define DECAY 1
#define SUSTAIN 2
#define RELEASE 3
#define OFF 4

typedef struct {
    uint8_t attack : 6;
    uint8_t decay : 6;
    uint8_t sustain : 4;
    uint8_t release : 6;
    uint8_t gate : 1;

    // used by envelope logic:
    uint8_t value : 4;
    uint8_t phase : 3;
    uint8_t counter;
    uint8_t gate_prev : 1;
} Envelope;

void envelope_update(Envelope* env);

Envelope env1;
Envelope env2;
Envelope env3;
