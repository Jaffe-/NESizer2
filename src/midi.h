#pragma once
#include <stdint.h>

extern uint8_t midi_channels[5];
extern uint8_t midi_transfer_progress;
extern uint8_t midi_notes[5];

void midi_handler();
