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
#include "parameter/parameter.h"
#include "apu/apu.h"
#include "envelope/envelope.h"
#include "lfo/lfo.h"
#include "modulation/modulation.h"
#include "patch/patch.h"
#include "portamento/portamento.h"
#include "assigner/assigner.h"

const struct parameter parameters[] PROGMEM = {
  [SQ1_ENABLED] = {&sq1.enabled, BOOL, 0, 0, 0},
  [SQ1_DUTY] = {&sq1.duty, RANGE, 0, 3, 2},
  [SQ1_GLIDE] = {&portamento_values[0], RANGE, 0, 99, 0},
  [SQ1_DETUNE] = {&mod_detune[0], RANGE, -9, 9, 0},
  [SQ1_LFO1] = {&mod_lfo_modmatrix[0][0], RANGE, 0, 99, 0},
  [SQ1_LFO2] = {&mod_lfo_modmatrix[0][1], RANGE, 0, 99, 0},
  [SQ1_LFO3] = {&mod_lfo_modmatrix[0][2], RANGE, 0, 99, 0},
  [SQ1_PITCHBEND] = {&mod_pitchbend[0], RANGE, 0, 24, 1},
  [SQ1_COARSE] = {&mod_coarse[0], RANGE, -36, 36, 0},
  
  [SQ2_ENABLED] = {&sq2.enabled, BOOL, 0, 0, 0},
  [SQ2_DUTY] = {&sq2.duty, RANGE, 0, 3, 2},
  [SQ2_GLIDE] = {&portamento_values[1], RANGE, 0, 99, 0},
  [SQ2_DETUNE] = {&mod_detune[1], RANGE, -9, 9, 0},
  [SQ2_LFO1] = {&mod_lfo_modmatrix[1][0], RANGE, 0, 99, 0},
  [SQ2_LFO2] = {&mod_lfo_modmatrix[1][1], RANGE, 0, 99, 0},
  [SQ2_LFO3] = {&mod_lfo_modmatrix[1][2], RANGE, 0, 99, 0},
  [SQ2_PITCHBEND] = {&mod_pitchbend[1], RANGE, 0, 24, 1},
  [SQ2_COARSE] = {&mod_coarse[1], RANGE, -36, 36, 0},
  
  [TRI_ENABLED] = {&tri.enabled, BOOL, 0, 0, 0},
  [TRI_GLIDE] = {&portamento_values[2], RANGE, 0, 99, 0},
  [TRI_DETUNE] = {&mod_detune[2], RANGE, -9, 9, 0},
  [TRI_LFO1] = {&mod_lfo_modmatrix[2][0], RANGE, 0, 99, 0},
  [TRI_LFO2] = {&mod_lfo_modmatrix[2][1], RANGE, 0, 99, 0},
  [TRI_LFO3] = {&mod_lfo_modmatrix[2][2], RANGE, 0, 99, 0},
  [TRI_PITCHBEND] = {&mod_pitchbend[2], RANGE, 0, 24, 1},
  [TRI_COARSE] = {&mod_coarse[2], RANGE, -36, 36, 0},
  [TRI_ENVMOD] = {&mod_envmod[2], RANGE, -9, 9, 0},

  [NOISE_ENABLED] = {&noise.enabled, BOOL, 0, 0, 0},
  [NOISE_LOOP] = {&noise.loop, RANGE, 0, 1,  0},
  [NOISE_LFO1] = {&mod_lfo_modmatrix[3][0], RANGE, 0, 99, 0},
  [NOISE_LFO2] = {&mod_lfo_modmatrix[3][1], RANGE, 0, 99, 0},
  [NOISE_LFO3] = {&mod_lfo_modmatrix[3][2], RANGE, 0, 99, 0},

  [DMC_ENABLED] = {&dmc.enabled, BOOL, 0, 1, 0},
  [DMC_SAMPLE_LOOP] = {&dmc.sample_loop, RANGE, 0, 1, 0},

  [ENV1_ATTACK] = {&env[0].attack, RANGE, 0, 99, 0},
  [ENV1_DECAY] = {&env[0].decay, RANGE, 0, 99, 0},
  [ENV1_SUSTAIN] = {&env[0].sustain, RANGE, 0, 15, 15},
  [ENV1_RELEASE] = {&env[0].release, RANGE, 0, 99, 0},

  [ENV2_ATTACK] = {&env[1].attack, RANGE, 0, 99, 0},
  [ENV2_DECAY] = {&env[1].decay, RANGE, 0, 99, 0},
  [ENV2_SUSTAIN] = {&env[1].sustain, RANGE, 0, 15, 15},
  [ENV2_RELEASE] = {&env[1].release, RANGE, 0, 99, 0},

  [ENV3_ATTACK] = {&env[2].attack, RANGE, 0, 99, 0},
  [ENV3_DECAY] = {&env[2].decay, RANGE, 0, 99, 0},
  [ENV3_SUSTAIN] = {&env[2].sustain, RANGE, 0, 15, 15},
  [ENV3_RELEASE] = {&env[2].release, RANGE, 0, 99, 0},

  [LFO1_PERIOD] = {&lfo[0].period, INVRANGE, 0, 99, 01},
  [LFO1_WAVEFORM] = {(int8_t*)&lfo[0].waveform, RANGE, 1, 5, 1},

  [LFO2_PERIOD] = {&lfo[1].period, INVRANGE, 0, 99, 01},
  [LFO2_WAVEFORM] = {(int8_t*)&lfo[1].waveform, RANGE, 1, 5, 1},

  [LFO3_PERIOD] = {&lfo[2].period, INVRANGE, 0, 99, 01},
  [LFO3_WAVEFORM] = {(int8_t*)&lfo[2].waveform, RANGE, 1, 5, 1},

  [ARP_RANGE] = {&assigner_arp_range, RANGE, 0, 3, 0},
  [ARP_MODE] = {(int8_t*)&assigner_arp_mode, RANGE, 1, 4, 1},
  [ARP_CHANNEL] = {&assigner_arp_channel, RANGE, 1, 16, 1},
  [ARP_RATE] = {&assigner_arp_rate, RANGE, 1, 99, 1},
  [ARP_SYNC] = {&assigner_arp_sync, BOOL, 0, 0, 0}
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
