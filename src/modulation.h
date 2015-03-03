#pragma once

uint16_t mod_periods[4];
uint8_t mod_lfo_modmatrix[4][3];
uint8_t mod_detune[3];
uint8_t mod_envmod[4];
uint16_t mod_pitchbend[4];

void mod_calculate();
void mod_apply();
int8_t get_envmod(uint8_t chn);
uint16_t mod_dc_to_T(uint16_t, int16_t);
