#include <avr/pgmspace.h>
#include "parameter.h"
#include "apu.h"
#include "envelope.h"
#include "lfo.h"
#include "modulation.h"
#include "patch.h"
#include "portamento.h"

const Parameter parameters[] PROGMEM = {
  {&sq1.enabled, VALTYPE_BOOL, 0, 0, 0},
  {&sq1.duty, VALTYPE_RANGE, 0, 3, 2},
  {&portamento_values[0], VALTYPE_RANGE, 0, 99, 0},
  {&mod_detune[0], VALTYPE_POLRANGE, 0, 18, 9},
  {&mod_envmod[0], VALTYPE_POLRANGE, 0, 18, 9},
  {&mod_lfo_modmatrix[0][0], VALTYPE_RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[0][1], VALTYPE_RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[0][2], VALTYPE_RANGE, 0, 99, 0},
  {&mod_pitchbend[0], VALTYPE_RANGE, 0, 12, 1},
  {&mod_coarse[0], VALTYPE_RANGE, 0, 36, 0},
  
  {&sq2.enabled, VALTYPE_BOOL, 0, 0, 0},
  {&sq2.duty, VALTYPE_RANGE, 0, 3, 2},
  {&portamento_values[1], VALTYPE_RANGE, 0, 99, 0},
  {&mod_detune[1], VALTYPE_POLRANGE, 0, 18, 9},
  {&mod_envmod[1], VALTYPE_POLRANGE, 0, 18, 9},
  {&mod_lfo_modmatrix[1][0], VALTYPE_RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[1][1], VALTYPE_RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[1][2], VALTYPE_RANGE, 0, 99, 0},
  {&mod_pitchbend[1], VALTYPE_RANGE, 0, 12, 1},
  {&mod_coarse[1], VALTYPE_RANGE, 0, 36, 0},
  
  {&tri.enabled, VALTYPE_BOOL, 0, 0, 0},
  {&portamento_values[2], VALTYPE_RANGE, 0, 99, 0},
  {&mod_detune[2], VALTYPE_POLRANGE, 0, 18, 9},
  {&mod_envmod[2], VALTYPE_POLRANGE, 0, 18, 9},
  {&mod_lfo_modmatrix[2][0], VALTYPE_RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[2][1], VALTYPE_RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[2][2], VALTYPE_RANGE, 0, 99, 0},
  {&mod_pitchbend[2], VALTYPE_RANGE, 0, 12, 1},
  {&mod_coarse[2], VALTYPE_RANGE, 0, 36, 0},

  {&noise.enabled, VALTYPE_BOOL, 0, 0, 0},
  {&noise.loop, VALTYPE_RANGE, 0, 1,  0},
  {&mod_envmod[3], VALTYPE_POLRANGE, 0, 18, 9},
  {&mod_lfo_modmatrix[3][0], VALTYPE_RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[3][1], VALTYPE_RANGE, 0, 99, 0},
  {&mod_lfo_modmatrix[3][2], VALTYPE_RANGE, 0, 99, 0},

  {&dmc.enabled, VALTYPE_BOOL, 0, 1, 0},
  {&dmc.sample_loop, VALTYPE_RANGE, 0, 1, 0},

  {&env1.attack, VALTYPE_RANGE, 0, 99, 0},
  {&env1.decay, VALTYPE_RANGE, 0, 99, 0},
  {&env1.sustain, VALTYPE_RANGE, 0, 15, 15},
  {&env1.release, VALTYPE_RANGE, 0, 99, 0},

  {&env2.attack, VALTYPE_RANGE, 0, 99, 0},
  {&env2.decay, VALTYPE_RANGE, 0, 99, 0},
  {&env2.sustain, VALTYPE_RANGE, 0, 15, 15},
  {&env2.release, VALTYPE_RANGE, 0, 99, 0},

  {&env3.attack, VALTYPE_RANGE, 0, 99, 0},
  {&env3.decay, VALTYPE_RANGE, 0, 99, 0},
  {&env3.sustain, VALTYPE_RANGE, 0, 15, 15},
  {&env3.release, VALTYPE_RANGE, 0, 99, 0},

  {&lfo1.period, VALTYPE_INVRANGE, 0, 99, 01},
  {&lfo1.waveform, VALTYPE_RANGE, 1, 5, 1},

  {&lfo2.period, VALTYPE_INVRANGE, 0, 99, 01},
  {&lfo2.waveform, VALTYPE_RANGE, 1, 5, 1},

  {&lfo3.period, VALTYPE_INVRANGE, 0, 99, 01},
  {&lfo3.waveform, VALTYPE_RANGE, 1, 5, 1},
};

Parameter parameter_get(ParameterID parameter)
{
  Parameter p = {pgm_read_ptr_near(&parameters[parameter].target),
		 pgm_read_byte_near(&parameters[parameter].type),
		 pgm_read_byte_near(&parameters[parameter].min),
		 pgm_read_byte_near(&parameters[parameter].max),
		 pgm_read_byte_near(&parameters[parameter].initial_value)};
  return p;
}
