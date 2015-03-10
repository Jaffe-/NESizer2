#pragma once

#include <stdint.h>

#define VALTYPE_BOOL 0
#define VALTYPE_RANGE 1
#define VALTYPE_INVRANGE 2
#define VALTYPE_POLRANGE 3

#define NUM_PARAMETERS 55

typedef struct {
    uint8_t* target;
    uint8_t type;
    uint8_t min;
    uint8_t max;
    uint8_t initial_value;
} Parameter;

typedef enum {
    SQ1_ENABLED,
    SQ1_DUTY,
    SQ1_GLIDE,
    SQ1_DETUNE,
    SQ1_ENVMOD,
    SQ1_LFO1,
    SQ1_LFO2,
    SQ1_LFO3,
    SQ1_PITCHBEND,
    SQ1_OCTAVE,

    SQ2_ENABLED,
    SQ2_DUTY,
    SQ2_GLIDE,
    SQ2_DETUNE,
    SQ2_ENVMOD,
    SQ2_LFO1,
    SQ2_LFO2,
    SQ2_LFO3,
    SQ2_PITCHBEND,
    SQ2_OCTAVE,

    TRI_ENABLED,
    TRI_GLIDE,
    TRI_DETUNE,
    TRI_ENVMOD,
    TRI_LFO1,
    TRI_LFO2,
    TRI_LFO3,
    TRI_PITCHBEND,
    TRI_OCTAVE,

    NOISE_ENABLED,
    NOISE_LOOP,
    NOISE_ENVMOD,
    NOISE_LFO1,
    NOISE_LFO2,
    NOISE_LFO3,

    DMC_ENABLED,
    DMC_SAMPLE_LOOP,

    ENV1_ATTACK,
    ENV1_DECAY,
    ENV1_SUSTAIN,
    ENV1_RELEASE,

    ENV2_ATTACK,
    ENV2_DECAY,
    ENV2_SUSTAIN,
    ENV2_RELEASE,

    ENV3_ATTACK,
    ENV3_DECAY,
    ENV3_SUSTAIN,
    ENV3_RELEASE,

    LFO1_PERIOD,
    LFO1_WAVEFORM,

    LFO2_PERIOD,
    LFO2_WAVEFORM,

    LFO3_PERIOD,
    LFO3_WAVEFORM
} ParameterID;

Parameter parameter_get(uint8_t num);
