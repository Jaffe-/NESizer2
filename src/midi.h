#pragma once
#include <stdint.h>

uint8_t midi_channels[5];
uint8_t midi_transfer_progress;

void midi_handler();
