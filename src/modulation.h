#pragma once

extern uint8_t mod_lfo_modmatrix[4][3];
extern uint8_t mod_detune[3];
extern uint8_t mod_envmod[4];
extern uint16_t mod_pitchbend_input[4];
extern uint8_t mod_pitchbend[3];
extern uint8_t noise_period;
extern uint8_t mod_coarse[3];

void mod_calculate();
void mod_apply();
int8_t get_envmod(uint8_t chn);
uint16_t mod_dc_to_T(uint16_t, int16_t);
