/*
  Copyright 2014-2015 Johan Fjeldtvedt 

  This file is part of NESIZER.

  NESIZER is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  NESIZER is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NESIZER.  If not, see <http://www.gnu.org/licenses/>.



  Modulation application

  Applies modulation to sound channels from various sources (LFOs,
  envelopes, portamento, pitchbend, tuning, etc.)
*/


#pragma once

extern int8_t mod_lfo_modmatrix[4][3];
extern int8_t mod_detune[3];
extern int8_t mod_envmod[4];
extern uint16_t mod_pitchbend_input[4];
extern int8_t mod_pitchbend[3];
extern uint8_t noise_period;
extern int8_t mod_coarse[3];
extern int8_t mod_pwm;

void mod_calculate(void);
void mod_apply(void);
//int8_t get_envmod(uint8_t chn);
uint16_t mod_dc_to_T(uint16_t, int16_t);
