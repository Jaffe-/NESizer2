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
    {&mod_lfo_modmatrix[1][0], VALTYPE_RANGE, 0, 99, 0},
    {&mod_lfo_modmatrix[2][0], VALTYPE_RANGE, 0, 99, 0},
    
    {&sq2.enabled, VALTYPE_BOOL, 0, 0, 0},
    {&sq2.duty, VALTYPE_RANGE, 0, 3, 2},
    {&portamento_values[1], VALTYPE_RANGE, 0, 99, 0},
    {&mod_detune[1], VALTYPE_POLRANGE, 0, 18, 9},
    {&mod_envmod[1], VALTYPE_POLRANGE, 0, 18, 9},
    {&mod_lfo_modmatrix[0][1], VALTYPE_RANGE, 0, 99, 0},
    {&mod_lfo_modmatrix[1][1], VALTYPE_RANGE, 0, 99, 0},
    {&mod_lfo_modmatrix[2][1], VALTYPE_RANGE, 0, 99, 0},

    {&tri.enabled, VALTYPE_BOOL, 0, 0, 0},
    {&portamento_values[2], VALTYPE_RANGE, 0, 99, 0},
    {&mod_detune[2], VALTYPE_POLRANGE, 0, 18, 9},
    {&mod_envmod[2], VALTYPE_POLRANGE, 0, 18, 9},
    {&mod_lfo_modmatrix[0][2], VALTYPE_RANGE, 0, 99, 0},
    {&mod_lfo_modmatrix[1][2], VALTYPE_RANGE, 0, 99, 0},
    {&mod_lfo_modmatrix[2][2], VALTYPE_RANGE, 0, 99, 0},

    {&noise.enabled, VALTYPE_BOOL, 0, 0, 0},
    {&noise.loop, VALTYPE_BOOL, 0, 0,  0},
    {&mod_envmod[3], VALTYPE_POLRANGE, 0, 18, 9},
    {&mod_lfo_modmatrix[0][3], VALTYPE_RANGE, 0, 99, 0},
    {&mod_lfo_modmatrix[1][3], VALTYPE_RANGE, 0, 99, 0},
    {&mod_lfo_modmatrix[2][3], VALTYPE_RANGE, 0, 99, 0},

    {&dmc.enabled, VALTYPE_BOOL, 0, 0, 0},
    {&dmc.sample_loop, VALTYPE_BOOL, 0, 0, 0},

    {&env1.attack, VALTYPE_RANGE, 0, 99, 0},
    {&env1.decay, VALTYPE_RANGE, 0, 99, 0},
    {&env1.sustain, VALTYPE_RANGE, 0, 15, 15},
    {&env1.release, VALTYPE_RANGE, 0, 99, 0},

    {&env2.attack, VALTYPE_RANGE, 0, 99, 0},
    {&env2.decay, VALTYPE_RANGE, 0, 99, 0},
    {&env2.sustain, VALTYPE_RANGE, 0, 15, 15},
    {&env2.release, VALTYPE_RANGE, 0, 99, 0},

    {&env2.attack, VALTYPE_RANGE, 0, 99, 0},
    {&env2.decay, VALTYPE_RANGE, 0, 99, 0},
    {&env2.sustain, VALTYPE_RANGE, 0, 15, 15},
    {&env2.release, VALTYPE_RANGE, 0, 99, 0},

    {&lfo1.period, VALTYPE_RANGE, 0, 99, 99},
    {&lfo1.waveform, VALTYPE_RANGE, 1, 3, 2},

    {&lfo2.period, VALTYPE_RANGE, 0, 99, 99},
    {&lfo2.waveform, VALTYPE_RANGE, 1, 3, 2},

    {&lfo3.period, VALTYPE_RANGE, 0, 99, 99},
    {&lfo3.waveform, VALTYPE_RANGE, 1, 3, 2},
};

Parameter parameter_get(ParameterID parameter)
{
    Parameter p = {pgm_read_ptr(&parameters[parameter].target),
		   pgm_read_byte(&parameters[parameter].type),
		   pgm_read_byte(&parameters[parameter].min),
		   pgm_read_byte(&parameters[parameter].max),
		   pgm_read_byte(&parameters[parameter].initial_value)};
    return p;
}
