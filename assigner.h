#pragma once

const uint16_t period_table[7][12];
uint16_t note_to_period(uint8_t channel, uint8_t note);
void play_note(uint8_t channel, uint8_t note);
void stop_note(uint8_t channel);
