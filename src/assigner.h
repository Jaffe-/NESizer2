#pragma once

uint16_t get_period(uint8_t chn, uint16_t c);
uint16_t note_to_period(uint8_t channel, uint8_t note);
void play_note(uint8_t channel, uint8_t note);
void stop_note(uint8_t channel);
