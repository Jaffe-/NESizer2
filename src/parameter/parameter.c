/*
  Copyright 2014-2016 Johan Fjeldtvedt

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
    [SQ1_ENABLED] = {&assigner_enabled[0], BOOL, 0, 0, 0},
    [SQ1_DUTY] = {&sq1.duty, RANGE, 0, 3, 2},
    [SQ1_GLIDE] = {&portamento_values[0], RANGE, 0, 99, 0},
    [SQ1_DETUNE] = {&mod_detune[0], RANGE, -9, 9, 0},
    [SQ1_LFO1] = {&mod_lfo_modmatrix[0][0], RANGE, 0, 99, 0},
    [SQ1_LFO2] = {&mod_lfo_modmatrix[0][1], RANGE, 0, 99, 0},
    [SQ1_LFO3] = {&mod_lfo_modmatrix[0][2], RANGE, 0, 99, 0},
    [SQ1_PITCHBEND] = {&mod_pitchbend[0], RANGE, 0, 24, 1},
    [SQ1_COARSE] = {&mod_octave[0], RANGE, -4, 4, 0},
    [SQ1_VOLMOD] = {&mod_lfo_vol[0], RANGE, 0, 16, 0},
    [SQ1_ENVMOD] = {&mod_envmod[0], RANGE, -9, 9, 0},
    [SQ1_HALF] = {&assigner_upper_mask[0], KBD_HALF, 0, 1, 1},

    [SQ2_ENABLED] = {&assigner_enabled[1], BOOL, 0, 0, 0},
    [SQ2_DUTY] = {&sq2.duty, RANGE, 0, 3, 2},
    [SQ2_GLIDE] = {&portamento_values[1], RANGE, 0, 99, 0},
    [SQ2_DETUNE] = {&mod_detune[1], RANGE, -9, 9, 0},
    [SQ2_LFO1] = {&mod_lfo_modmatrix[1][0], RANGE, 0, 99, 0},
    [SQ2_LFO2] = {&mod_lfo_modmatrix[1][1], RANGE, 0, 99, 0},
    [SQ2_LFO3] = {&mod_lfo_modmatrix[1][2], RANGE, 0, 99, 0},
    [SQ2_PITCHBEND] = {&mod_pitchbend[1], RANGE, 0, 24, 1},
    [SQ2_COARSE] = {&mod_octave[1], RANGE, -4, 4, 0},
    [SQ2_VOLMOD] = {&mod_lfo_vol[1], RANGE, 0, 16, 0},
    [SQ2_ENVMOD] = {&mod_envmod[1], RANGE, -9, 9, 0},
    [SQ2_HALF] = {&assigner_upper_mask[1], KBD_HALF, 0, 1, 1},

    [TRI_ENABLED] = {&assigner_enabled[2], BOOL, 0, 0, 0},
    [TRI_GLIDE] = {&portamento_values[2], RANGE, 0, 99, 0},
    [TRI_DETUNE] = {&mod_detune[2], RANGE, -9, 9, 0},
    [TRI_LFO1] = {&mod_lfo_modmatrix[2][0], RANGE, 0, 99, 0},
    [TRI_LFO2] = {&mod_lfo_modmatrix[2][1], RANGE, 0, 99, 0},
    [TRI_LFO3] = {&mod_lfo_modmatrix[2][2], RANGE, 0, 99, 0},
    [TRI_PITCHBEND] = {&mod_pitchbend[2], RANGE, 0, 24, 1},
    [TRI_COARSE] = {&mod_octave[2], RANGE, -4, 4, 0},
    [TRI_ENVMOD] = {&mod_envmod[2], RANGE, -9, 9, 0},
    [TRI_HALF] = {&assigner_upper_mask[2], KBD_HALF, 0, 1, 0},

    [NOISE_ENABLED] = {&assigner_enabled[3], BOOL, 0, 0, 0},
    [NOISE_LOOP] = {&noise.loop, RANGE, 0, 1,  0},
    [NOISE_LFO1] = {&mod_lfo_modmatrix[3][0], RANGE, 0, 99, 0},
    [NOISE_LFO2] = {&mod_lfo_modmatrix[3][1], RANGE, 0, 99, 0},
    [NOISE_LFO3] = {&mod_lfo_modmatrix[3][2], RANGE, 0, 99, 0},
    [NOISE_VOLMOD] = {&mod_lfo_vol[2], RANGE, 0, 16, 0},
    [NOISE_ENVMOD] = {&mod_envmod[3], RANGE, -9, 9, 0},
    [NOISE_HALF] = {&assigner_upper_mask[3], KBD_HALF, 0, 1, 1},

    [DMC_ENABLED] = {&assigner_enabled[4], BOOL, 0, 1, 0},
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

    [SPLIT_POINT] = {(int8_t*)&assigner_split_point, NOTE, 24, 84, 48}
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
