#include <avr/pgmspace.h>
#include <stdint.h>
#include "midi_cc.h"
#include "midi.h"
#include "assigner/assigner.h"

/*
   Midi CC to parameter table
*/
#define MAX_PULSE_STATES 13
#define MAX_TRIANGLE_STATES 7
#define MAX_NOISE_STATES 10
#define MAX_DMC_STATES 1

struct state_toggle pulse1_state[MAX_PULSE_STATES];
struct state_toggle pulse2_state[MAX_PULSE_STATES];
struct state_toggle triangle_state[MAX_TRIANGLE_STATES];
struct state_toggle noise_state[MAX_NOISE_STATES];
struct state_toggle dmc_state[MAX_DMC_STATES];

size_t state_sizes[] = {
    sizeof(pulse1_state) / sizeof(pulse1_state[0]),
    sizeof(pulse2_state) / sizeof(pulse2_state[0]),
    sizeof(triangle_state) / sizeof(triangle_state[0]),
    sizeof(noise_state) / sizeof(noise_state[0]),
    sizeof(dmc_state) / sizeof(dmc_state[0])
};

void state_toggle_init(struct state_toggle *arg, size_t ln)
{
    size_t i;
    ln = state_sizes[ln];
    for (i = 0; i < ln; i++) {
        arg[i].stashed = false;
    }
}

void midi_init(void)
{
    uint8_t i = 0;
    state_toggle_init(pulse1_state, i++);
    state_toggle_init(pulse2_state, i++);
    state_toggle_init(triangle_state, i++);
    state_toggle_init(noise_state, i++);
    state_toggle_init(dmc_state, i);
}


// 32 - 63 Undefined cc

const struct midi_command pulse1_cc[] PROGMEM = {
    // {0, NULL},  // TODO bank select,
    {1,  SQ1_DUTY},
    {5,  SQ1_GLIDE, 65, &pulse1_state[1].state, &pulse1_state[1].stashed},

    {24, SQ1_ENVMOD},  // Pitch envelope modulation amount

    {30, SQ1_LFO1},  // 77
    {31, SQ1_LFO2},
    {32, SQ1_LFO3},

    {72, ENV1_RELEASE, 60, &pulse1_state[9].state,  &pulse1_state[9].stashed},
    {73, ENV1_ATTACK,  61, &pulse1_state[10].state, &pulse1_state[10].stashed},
    {75, ENV1_DECAY,   62, &pulse1_state[11].state, &pulse1_state[11].stashed},

    {77, SQ1_VOLMOD},  // Volume modulation by LFO3

    {79, ENV1_SUSTAIN, 63, &pulse1_state[12].state, &pulse1_state[12].stashed},

    {82, SQ1_PITCHBEND},  // Bend wheel intensity in semitones
    {85, SQ1_DETUNE},
    {94, SQ1_COARSE},  // Octave shift
    // 121 rest values
    {123, SQ1_ENABLED},
};

const struct midi_command pulse2_cc[] PROGMEM = {
    // {0, NULL},  // TODO bank select
    {1,  SQ2_DUTY},
    {5,  SQ2_GLIDE, 65, &pulse2_state[1].state, &pulse2_state[1].stashed},

    {24, SQ2_ENVMOD},  // Pitch envelope modulation amount

    {30, SQ2_LFO1},  // 77
    {31, SQ2_LFO2},
    {32, SQ2_LFO3},

    {72, ENV2_RELEASE, 60, &pulse2_state[9].state,  &pulse2_state[9].stashed},
    {73, ENV2_ATTACK,  61, &pulse2_state[10].state, &pulse2_state[10].stashed},
    {75, ENV2_DECAY,   62, &pulse2_state[11].state, &pulse2_state[11].stashed},

    {77, SQ2_VOLMOD},  // Volume modulation by LFO3

    {79, ENV2_SUSTAIN, 63, &pulse2_state[12].state, &pulse2_state[12].stashed},

    {82, SQ2_PITCHBEND},  // Bend wheel intensity in semitones
    {85, SQ2_DETUNE},
    {94, SQ2_COARSE},  // Octave shift
    // 121 rest values
    {123, SQ2_ENABLED},
};

const struct midi_command triangle_cc[] PROGMEM = {
    // {0, NULL},  // TODO bank select
    {5, TRI_GLIDE, 65, &triangle_state[0].state, &triangle_state[0].stashed},
    // {TRI_PITCHBEND},
    {24, TRI_ENVMOD},

    {30, TRI_LFO1},
    {31, TRI_LFO2},
    {32, TRI_LFO3},

    {82, TRI_PITCHBEND},  // Bend wheel intensity in semitones
    {85, TRI_DETUNE},

    {94, TRI_COARSE},

    {123, TRI_ENABLED},
};

const struct midi_command noise_cc[] PROGMEM = {
    // {0, NULL},  // TODO bank select

    {14, NOISE_LOOP},

    {24, NOISE_ENVMOD},
    {30, NOISE_LFO1},
    {31, NOISE_LFO2},
    {32, NOISE_LFO3},

    {72, ENV3_RELEASE},
    {73, ENV3_ATTACK},
    {75, ENV3_DECAY},

    {77, NOISE_VOLMOD},

    {79, ENV3_SUSTAIN},

    {82, NOISE_PITCHBEND},  // Bend wheel intensity in semitones

    {123, NOISE_ENABLED},
};

const struct midi_command dmc_cc[] PROGMEM = {
    {14, DMC_SAMPLE_LOOP},
    {123, DMC_ENABLED},
};


/*
   Global CC table
*/
const struct midi_command global_cc[] PROGMEM = {


    //TODO move to a global midi channel


    {50, LFO1_PERIOD},  // 76
    {51, LFO1_WAVEFORM},  // 80

    {52, LFO2_PERIOD},
    {53, LFO2_WAVEFORM},

    {54, LFO3_PERIOD},
    {55, LFO3_WAVEFORM},
};

const uint8_t midi_channels_cc_lengths[] PROGMEM = {
    sizeof(pulse1_cc) / sizeof(pulse1_cc[0]),
    sizeof(pulse2_cc) / sizeof(pulse2_cc[0]),
    sizeof(triangle_cc) / sizeof(triangle_cc[0]),
    sizeof(noise_cc) / sizeof(noise_cc[0]),
    sizeof(dmc_cc) / sizeof(dmc_cc[0]),
    sizeof(global_cc) / sizeof(global_cc[0])
};

const struct midi_command * const midi_channels_cc[] PROGMEM = {pulse1_cc, pulse2_cc, triangle_cc, noise_cc, dmc_cc, global_cc};


/*
    Resets all values on a channel
*/
void midi_channels_cc_reset(uint8_t chn)
{
    // return;  // Not ready yet

    struct midi_command * m = pgm_read_ptr_near(&midi_channels_cc[chn]);
    uint8_t ln = pgm_read_byte_near(&midi_channels_cc_lengths[chn]);
    struct parameter param;

    for (uint8_t i = 0; i < ln; i++) {
        param = parameter_get(*&m[i].parameter);
        *param.target = param.initial_value;
    }
}

/*
    Resets all values on a all channels
*/
void midi_channels_cc_reset_all()
{
    for (int8_t i = 0; i < 5; i++) {
        midi_channels_cc_reset(i);
    }
}

/*
    Matches the CC channel to the internal parameter.
    Return: index
*/
int8_t midi_command_get_cc(uint8_t chn, uint8_t data1)
{
    struct midi_command * m = pgm_read_ptr_near(&midi_channels_cc[chn]);
    uint8_t ln = pgm_read_byte_near(&midi_channels_cc_lengths[chn]);
    uint8_t mem_cc;
    int8_t mem_cc_toggle;
    int8_t i;

    for (i = 0; i < ln; i++) {
        mem_cc = pgm_read_byte_near(&m[i].cc);
        mem_cc_toggle = pgm_read_byte_near(&m[i].cc_toggle);

        if (data1 == mem_cc || data1 == mem_cc_toggle) {
            return i;
        }
    }

    return -1;
}

struct midi_command midi_command_get(uint8_t chn, int8_t index) {
    struct midi_command * m = pgm_read_ptr_near(&midi_channels_cc[chn]);

    struct midi_command mc = {
        pgm_read_byte_near(&m[index].cc),
        pgm_read_byte_near(&m[index].parameter),
        pgm_read_byte_near(&m[index].cc_toggle),
        pgm_read_ptr_near(&m[index].stashed_state),
        pgm_read_ptr_near(&m[index].stash_active)
    };

    return mc;
}

int8_t map_cc(long x, long out_min, long out_max)
{
    return x * (out_max - out_min) / 127 + out_min;
}

uint8_t get_target_value(struct parameter parameter, int8_t cc_value)
{
    // uint8_t total_range = 0;
    uint8_t target_value = -1;

    // TODO might not need if we dont change the range
    int8_t min = parameter.min;
    int8_t max = parameter.max;

    // TODO
    switch (parameter.type) {
        case BOOL:
            if (cc_value < 0x40) {
                target_value = 0;
            }
            target_value = 1;
            // max = 1;  // Might need to be 2
            // Set range (on/off):(0-63/64-127) for BOOL
            // total_range = 2;
            break;

        case RANGE:
        case INVRANGE:
            // if (total_range > 0x40) {  // 64
            //     total_range = MIDI_MAX_CC;
            // } else {
            //     total_range = parameter.max - parameter.min + 1;
            // }
            // range_offset = parameter.min;
            target_value = map_cc(cc_value, min, max);
            break;

        default:
            // NOTE, KBD_HALF, SCALE
            target_value = 0;
            // return target_value;
    }

    return target_value;
}

void control_change(uint8_t midi_chn, uint8_t data1, uint8_t data2)
{
    uint8_t chn = assigner_channel_get(midi_chn);

    // Allow LFO updates on any channel
    if (data1 > 49 && data1 < 56) {
        chn = 5;
    }

    // This will be moved into the new objects
    if (chn < 5 && data1 == 121 && data2 > 63) {
        return midi_channels_cc_reset(chn);
    }

    // if (chn < 0) {
    //     return;
    // }

    int8_t cc_index = midi_command_get_cc(chn, data1);
    if (cc_index < 0) {
        return;
    }

    struct midi_command command = midi_command_get(chn, cc_index);
    struct parameter parameter = parameter_get(command.parameter);
    uint8_t target_value = get_target_value(parameter, data2);

    // Check for toggle state
    if (data1 == command.cc_toggle) {
        if (data2 > MIDI_MID_CC && *command.stash_active) {  // data1 > 63
            // Enable cc parameter
            *parameter.target = *command.stashed_state;
            *command.stash_active = false;
            return;
        }
        if (data2 < MIDI_MID_CC && ! *command.stash_active) {
            // Turn off parameter and stash
            *command.stash_active = true;
            *command.stashed_state = *parameter.target;
            *parameter.target = parameter.initial_value;
            return;
        }
        return;
    }

    if (*command.stash_active) {
        *command.stashed_state  = target_value;
        return;
    }

    *parameter.target = target_value;
}