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



  Programmable parameters

  Contains the list of parameters that can be programmed in a patch. 
*/


#include <avr/pgmspace.h>
#include "parameter.h"
#include "apu.h"
#include "envelope.h"
#include "lfo.h"
#include "modulation.h"
#include "patch.h"
#include "portamento.h"

const struct parameter parameters[] PROGMEM = {
  {&sq1.enabled, BOOL, 0, 0, 0},
  {&sq1.duty, RANGE, 0, 3, 2},
  {&portamento_values[0], RANGE, 0, 99, 0},
  {&mod_detune[0], RANGE, -9, 9, 0},
  {&mod_envmod[0], RANGE, -9, 9, 0},
  {&mod_lfo_modmatrix[0][0], RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[0][1], RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[0][2], RANGE, 0, 99, 0},
  {&mod_pitchbend[0], RANGE, 0, 12, 1},
  {&mod_coarse[0], RANGE, 0, 36, 0},
  
  {&sq2.enabled, BOOL, 0, 0, 0},
  {&sq2.duty, RANGE, 0, 3, 2},
  {&portamento_values[1], RANGE, 0, 99, 0},
  {&mod_detune[1], RANGE, -9, 9, 0},
  {&mod_envmod[1], RANGE, -9, 9, 0},
  {&mod_lfo_modmatrix[1][0], RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[1][1], RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[1][2], RANGE, 0, 99, 0},
  {&mod_pitchbend[1], RANGE, 0, 12, 1},
  {&mod_coarse[1], RANGE, 0, 36, 0},
  
  {&tri.enabled, BOOL, 0, 0, 0},
  {&portamento_values[2], RANGE, 0, 99, 0},
  {&mod_detune[2], RANGE, -9, 9, 0},
  {&mod_envmod[2], RANGE, -9, 9, 0},
  {&mod_lfo_modmatrix[2][0], RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[2][1], RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[2][2], RANGE, 0, 99, 0},
  {&mod_pitchbend[2], RANGE, 0, 12, 1},
  {&mod_coarse[2], RANGE, 0, 36, 0},

  {&noise.enabled, BOOL, 0, 0, 0},
  {&noise.loop, RANGE, 0, 1,  0},
  {&mod_envmod[3], RANGE, -9, 9, 0},
  {&mod_lfo_modmatrix[3][0], RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[3][1], RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[3][2], RANGE, 0, 99, 0},

  {&dmc.enabled, BOOL, 0, 1, 0},
  {&dmc.sample_loop, RANGE, 0, 1, 0},

  {&env1.attack, RANGE, 0, 99, 0},
  {&env1.decay, RANGE, 0, 99, 0},
  {&env1.sustain, RANGE, 0, 15, 15},
  {&env1.release, RANGE, 0, 99, 0},

  {&env2.attack, RANGE, 0, 99, 0},
  {&env2.decay, RANGE, 0, 99, 0},
  {&env2.sustain, RANGE, 0, 15, 15},
  {&env2.release, RANGE, 0, 99, 0},

  {&env3.attack, RANGE, 0, 99, 0},
  {&env3.decay, RANGE, 0, 99, 0},
  {&env3.sustain, RANGE, 0, 15, 15},
  {&env3.release, RANGE, 0, 99, 0},

  {&lfo1.period, INVRANGE, 0, 99, 01},
  {(int8_t*)&lfo1.waveform, RANGE, 1, 5, 1},

  {&lfo2.period, INVRANGE, 0, 99, 01},
  {(int8_t*)&lfo2.waveform, RANGE, 1, 5, 1},

  {&lfo3.period, INVRANGE, 0, 99, 01},
  {(int8_t*)&lfo3.waveform, RANGE, 1, 5, 1},
};

struct parameter parameter_get(enum parameter_id parameter)
{
  struct parameter p = {pgm_read_ptr_near(&parameters[parameter].target),
			pgm_read_byte_near(&parameters[parameter].type),
			pgm_read_byte_near(&parameters[parameter].min),
			pgm_read_byte_near(&parameters[parameter].max),
			pgm_read_byte_near(&parameters[parameter].initial_value)};
  return p;
}
