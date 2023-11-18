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



  MIDI interpreter

  Interprets MIDI messages and acts accordingly.
*/

#include <stdbool.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include "midi.h"
#include "io/midi.h"
#include "envelope/envelope.h"
#include "lfo/lfo.h"
#include "ui/ui.h"
#include "ui/ui_sequencer.h"
#include "modulation/modulation.h"
#include "io/leds.h"
#include "portamento/portamento.h"
#include "assigner/assigner.h"
#include "sample/sample.h"
#include "sequencer/sequencer.h"

enum midi_state {
    STATE_MESSAGE,
    STATE_SYSEX,
    STATE_TRANSFER
};

uint8_t midi_notes[5];

static enum midi_state state = STATE_MESSAGE;
static uint8_t sysex_command;

static inline void interpret_message();
static inline void transfer();
static inline void sysex();

static inline void initiate_transfer();
static inline uint8_t get_midi_channel(uint8_t channel)
{
    return channel + 1;
}

/* Apply a new message */
void midi_channel_apply(struct midi_message* msg)
{
    uint8_t midi_channel = get_midi_channel(msg->channel);
    switch (msg->command) {
    case MIDI_CMD_NOTE_ON:
        if (getvalue.state == ACTIVE && getvalue.parameter.type == NOTE) {
            getvalue.midi_note = msg->data1;
        } else {
            sequencer_midi_note = msg->data1;
            if (msg->data2 == 0) {
                assigner_notify_note_off(midi_channel, msg->data1);
            } else {
                assigner_notify_note_on(midi_channel, msg->data1);
            }
        }
        break;

    case MIDI_CMD_NOTE_OFF:
        if (sequencer_midi_note == msg->data1)
            sequencer_midi_note = 0xFF;
        assigner_notify_note_off(midi_channel, msg->data1);
        break;

    case MIDI_CMD_PITCH_BEND:
        for (uint8_t i = 0; i < 5; i++) {
            if (assigner_midi_channel_get(i) == midi_channel) {
                if (i < 3)
                    mod_pitchbend_input[i] = ((uint16_t)msg->data1) | ((uint16_t)msg->data2) << 7;
                else if (i == 3)
                    mod_pitchbend_input[i] = msg->data2 >> 3;
            }
        }
        break;
    case MIDI_CMD_CONTROL_CHANGE:
        control_change( midi_channel, msg->data1, msg->data2 );
        break;
    }
}

void midi_handler()
{
    switch (state) {
    case STATE_MESSAGE:
        interpret_message(); break;
    case STATE_SYSEX:
        sysex(); break;
    case STATE_TRANSFER:
        transfer(); break;
    default: break;
    }
}

static inline void interpret_message()
{
    while (midi_io_buffer_nonempty()) {
        struct midi_message msg = {0};
        if (!midi_io_read_message(&msg))
            continue;

        if (midi_is_channel_message(msg.command)) {
            midi_channel_apply(&msg);
            continue;
        }

        /* Not channel directed message */
        switch (msg.command) {
        case MIDI_CMD_SYSEX:
            state = STATE_SYSEX;
            break;

        case MIDI_CMD_TIMECODE:
            break;

        case MIDI_CMD_SONGPOS:
            break;

        case MIDI_CMD_SONGSEL:
            break;

        case MIDI_CMD_TUNEREQUEST:
            break;

        case MIDI_CMD_SYSEX_END:
            break;

        case MIDI_CMD_CLOCK:
            sequencer_midi_clock();
            break;

        case MIDI_CMD_START:
            sequencer_play();
            break;

        case MIDI_CMD_CONTINUE:
            sequencer_continue();
            break;

        case MIDI_CMD_STOP:
            sequencer_stop();
            break;

        case MIDI_CMD_ACTIVESENSE:
            break;

        case MIDI_CMD_RESET:
            break;
        }
    }
}

#define SYSEX_STOP 0xF7

uint8_t midi_transfer_progress = 0;

static struct sample sample;

static inline void sysex()
{
    // When the state just changed, we need to look at the first few bytes
    // to determine what sysex message we're dealing with
    if (midi_io_bytes_remaining() >= 6) {
        sysex_command = midi_io_read_byte();

        if (sysex_command == SYSEX_CMD_SAMPLE_LOAD) {

            // Read sample descriptor and store in sample object
            uint8_t sample_number = midi_io_read_byte();
            sample.type = midi_io_read_byte();
            sample.size = midi_io_read_byte();
            sample.size |= (uint32_t)midi_io_read_byte() << 7;
            sample.size |= (uint32_t)midi_io_read_byte() << 14;
            sample_new(&sample, sample_number);

            initiate_transfer();
        }
    }
}

#define ERROR_MIDI_RX_LEN_MISMATCH (1 << 2)

static inline void transfer()
/*
  Handlers transfering of data via MIDI
*/
{
    static uint8_t nibble_flag = 0;
    // static uint8_t temp_val = 0;

    while (midi_io_bytes_remaining() >= 1) {
        uint8_t val = midi_io_read_byte();

        if (val == SYSEX_STOP) {
            ui_pop_mode();
            state = STATE_MESSAGE;
            if (sample.bytes_done != sample.size)
                error_set(ERROR_MIDI_RX_LEN_MISMATCH);
        }

        else if ((val & 0x80) == 0) {
            if (sysex_command == SYSEX_CMD_SAMPLE_LOAD) {
                if (sample.bytes_done < sample.size) {
                    sample_write_serial(&sample, val);
                    midi_transfer_progress = (sample.bytes_done << 4) / sample.size;
                }
            }
        }
    }
}

static inline void initiate_transfer()
{
    // Set UI mode to transfer (which turns the button LEDs into a status bar
    // for the duration of the transfer)
    ui_push_mode(MODE_TRANSFER);

    // Disable DMC
    dmc.sample_enabled = 0;

    state = STATE_TRANSFER;
    midi_transfer_progress = 0;
}

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
    sizeof(pulse1_state)/sizeof(pulse1_state[0]),
    sizeof(pulse2_state)/sizeof(pulse2_state[0]),
    sizeof(triangle_state)/sizeof(triangle_state[0]),
    sizeof(noise_state)/sizeof(noise_state[0]),
    sizeof(dmc_state)/sizeof(dmc_state[0])
};

void state_toggle_init(struct state_toggle *arg, size_t ln )
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
    state_toggle_init(pulse1_state, i++ );
    state_toggle_init(pulse2_state, i++);
    state_toggle_init(triangle_state, i++);
    state_toggle_init(noise_state, i++);
    state_toggle_init(dmc_state, i);
}


// 32 - 63 Undefined cc

const struct midi_command pulse1_cc[] PROGMEM = {
    // {0, NULL}, //TODO bank select,
    {1, SQ1_DUTY},
    {5, SQ1_GLIDE, 65, &pulse1_state[1].state, &pulse1_state[1].stashed },

    {24, SQ1_ENVMOD}, //Pitch envelope modulation amount
    
    {30, SQ1_LFO1}, //77
    {31, SQ1_LFO2},
    {32, SQ1_LFO3},

    {72, ENV1_RELEASE, 60, &pulse1_state[9].state, &pulse1_state[9].stashed },
    {73, ENV1_ATTACK,  61, &pulse1_state[10].state, &pulse1_state[10].stashed },
    {75, ENV1_DECAY,   62, &pulse1_state[11].state, &pulse1_state[11].stashed },
    
    {77, SQ1_VOLMOD}, //Volume modulation by LFO3

    {79, ENV1_SUSTAIN, 63, &pulse1_state[12].state, &pulse1_state[12].stashed },
    
    {82, SQ1_PITCHBEND}, //Bend wheel intensity in semitones
    {85, SQ1_DETUNE},
    {94, SQ1_COARSE}, //Octave shift
    // 121 rest values//
    {123, SQ1_ENABLED}, //
};
const struct midi_command pulse2_cc[] PROGMEM = {
    // {0, NULL}, //TODO bank select,
    {1, SQ2_DUTY},
    {5, SQ2_GLIDE, 65, &pulse2_state[1].state, &pulse2_state[1].stashed },

    {24, SQ2_ENVMOD}, //Pitch envelope modulation amount

    {30, SQ2_LFO1}, //77
    {31, SQ2_LFO2},
    {32, SQ2_LFO3},

    {72, ENV2_RELEASE, 60, &pulse2_state[9].state, &pulse2_state[9].stashed },
    {73, ENV2_ATTACK,  61, &pulse2_state[10].state, &pulse2_state[10].stashed },
    {75, ENV2_DECAY,   62, &pulse2_state[11].state, &pulse2_state[11].stashed },

    {77, SQ2_VOLMOD}, //Volume modulation by LFO3

    {79, ENV2_SUSTAIN, 63, &pulse2_state[12].state, &pulse2_state[12].stashed },

    {82, SQ2_PITCHBEND}, //Bend wheel intensity in semitones
    {85, SQ2_DETUNE},
    {94, SQ2_COARSE}, //Octave shift
    // 121 rest values//
    {123, SQ2_ENABLED}, //
};
const struct midi_command triangle_cc[] PROGMEM = {
    // {0, NULL}, //TODO bank select,
    {5, TRI_GLIDE, 65,  &triangle_state[0].state, &triangle_state[0].stashed},
    // {TRI_PITCHBEND},
    {24, TRI_ENVMOD},
    
    {30, TRI_LFO1},
    {31, TRI_LFO2},
    {32, TRI_LFO3},

    {82, TRI_PITCHBEND}, //Bend wheel intensity in semitones
    {85, TRI_DETUNE},
    
    {94, TRI_COARSE},

    {123, TRI_ENABLED},
};
const struct midi_command noise_cc[] PROGMEM = {
    // {0, NULL}, //TODO bank select,
    
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

    {82, NOISE_PITCHBEND}, //Bend wheel intensity in semitones

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


    {50, LFO1_PERIOD}, // 76
    {51, LFO1_WAVEFORM}, //80

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

const struct midi_command * const midi_channels_cc[] PROGMEM = { pulse1_cc, pulse2_cc, triangle_cc, noise_cc, dmc_cc, global_cc };

/*
    Resets all values on a channel
*/
void midi_channels_cc_reset(uint8_t chn)
{
    // return; //Not ready yet

    struct midi_command * m = pgm_read_ptr_near( &midi_channels_cc[chn] );
    uint8_t ln = pgm_read_byte_near(&midi_channels_cc_lengths[chn]);
    struct parameter param;

    for (uint8_t i = 0; i < ln ; i++ ) {
        param = parameter_get( *&m[i].parameter );
        *param.target = param.initial_value;
    }
}
/*
    Resets all values on a all channels
*/
void midi_channels_cc_reset_all(){
    for (int8_t i = 0; i < 5; i++) {
        midi_channels_cc_reset(i);
    }
}

/*
    Matches the CC channel to the internal parameter.
    Return: index
*/
int8_t midi_command_get_cc(uint8_t chn, uint8_t data1 )
{
    struct midi_command * m = pgm_read_ptr_near( &midi_channels_cc[chn] );
    uint8_t ln = pgm_read_byte_near(&midi_channels_cc_lengths[chn]);
    uint8_t mem_cc;
    int8_t mem_cc_toggle;
    int8_t i;
    for ( i = 0; i < ln ; i++ ) {
        mem_cc = pgm_read_byte_near(&m[i].cc);
        mem_cc_toggle = pgm_read_byte_near(&m[i].cc_toggle);

        if (data1 == mem_cc || data1 == mem_cc_toggle) {
            return i;
        }
    }

    return -1;
}

struct midi_command midi_command_get(uint8_t chn, int8_t index )
{
    struct midi_command * m = pgm_read_ptr_near( &midi_channels_cc[chn] );

    struct midi_command mc = {
        pgm_read_byte_near(&m[index].cc),
        pgm_read_byte_near(&m[index].parameter),
        pgm_read_byte_near(&m[index].cc_toggle),
        pgm_read_ptr_near(&m[index].stashed_state),
        pgm_read_ptr_near(&m[index].stash_active)
    };

    return mc;
}

int8_t map_cc(long x, long out_min, long out_max) {
  return x * (out_max - out_min) / 127 + out_min;
}

uint8_t get_target_value(struct parameter parameter, int8_t cc_value ){
    // uint8_t total_range = 0;
    uint8_t target_value = -1;

    //TODO might not need if we dont change the range
    int8_t min = parameter.min;
    int8_t max = parameter.max;

    //TODO 
    switch (parameter.type) {
    case BOOL:
        if (cc_value < 0x40) {
            target_value = 0;
        }
        target_value = 1;
        // max = 1; //Might need to be 2
        //Set range (on/off):(0-63/64-127) for BOOL
        // total_range = 2;
        break;
    case RANGE:
    case INVRANGE:
        // if (total_range > 0x40) { //64
        //     total_range = MIDI_MAX_CC;
        // }else {
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
    uint8_t chn = assigner_channel_get( midi_chn );

    //Allow LFO updates on any channel
    if (data1 > 49 && data1 < 56) {
        chn = 5;
    }

    //This will be moved into the new objects
    if (chn < 5 && data1 == 121 && data2 > 63) {
        return midi_channels_cc_reset(chn);
    }

    // if (chn < 0) {
    //     return;
    // }

    int8_t cc_index = midi_command_get_cc( chn, data1 );
    if (cc_index < 0) {
        return;
    }

    struct midi_command command = midi_command_get( chn, cc_index );
    struct parameter parameter = parameter_get( command.parameter );
    uint8_t target_value = get_target_value(parameter, data2);

    //Check for toggle state
    if (data1 == command.cc_toggle) {
        if (data2 > MIDI_MID_CC && *command.stash_active) { //data1 > 63
            //Enable cc parameter
            *parameter.target = *command.stashed_state;
            *command.stash_active = false;
            return;
        } 
        if (data2 < MIDI_MID_CC && ! *command.stash_active) {
            //Turn off parameter and stash
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