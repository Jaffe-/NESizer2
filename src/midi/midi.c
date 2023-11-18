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
    static uint8_t temp_val = 0;

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

const struct midi_command midi_channels_cc[][15] = {
    [0] = {
        // {0, NULL}, //TODO bank select,
        {1, SQ1_DUTY},
        {5, SQ1_GLIDE},
        {7, SQ1_VOLMOD},
        {14, SQ1_DETUNE},
        {15, SQ1_COARSE},
        {20, ENV1_ATTACK},
        {21, ENV1_DECAY},
        {22, ENV1_SUSTAIN},
        {23, ENV1_RELEASE},   
        {24, SQ1_ENVMOD},
        {30, SQ1_LFO1},
        {31, SQ1_LFO2},
        {32, SQ1_LFO3},
        {123, SQ1_ENABLED},
    },
    [1] = {
        // {0, NULL}, //TODO bank select,
        {1, SQ2_DUTY},
        {5, SQ2_GLIDE},
        {7, SQ2_VOLMOD},
        {14, SQ2_DETUNE},
        {15, SQ2_COARSE},
        {20, ENV2_ATTACK},
        {21, ENV2_DECAY},
        {22, ENV2_SUSTAIN},
        {23, ENV2_RELEASE},   
        {24, SQ2_ENVMOD},
        {30, SQ2_LFO1},
        {31, SQ2_LFO2},
        {32, SQ2_LFO3},
        {123, SQ2_ENABLED},
    },
    [2] = {
        // {0, NULL}, //TODO bank select,        
        {5, TRI_GLIDE},
        // {TRI_PITCHBEND},
        {14, TRI_DETUNE},
        {15, TRI_COARSE},
        {24, TRI_ENVMOD},
        {30, TRI_LFO1},
        {31, TRI_LFO2},
        {32, TRI_LFO3},
        {123, TRI_ENABLED},
    },
    [3] = {
        // {0, NULL}, //TODO bank select,
        {7, NOISE_VOLMOD},
        {14, NOISE_LOOP},
        {20, ENV3_ATTACK},
        {21, ENV3_DECAY},
        {22, ENV3_SUSTAIN},
        {23, ENV3_RELEASE},  
        {24, NOISE_ENVMOD},
        {30, NOISE_LFO1},
        {31, NOISE_LFO2},
        {32, NOISE_LFO3},
        {123, NOISE_ENABLED},
    },
    [4] = {
        {14, DMC_SAMPLE_LOOP},
        {123, DMC_ENABLED},
    },
    [5] = {
        //Global CC
        {50, LFO1_PERIOD},
        {51, LFO1_WAVEFORM},

        {52, LFO2_PERIOD},
        {53, LFO2_WAVEFORM},

        {54, LFO3_PERIOD},
        {55, LFO3_WAVEFORM},
    }
};

enum parameter_id get_cc_parameter( uint8_t chn, uint8_t cc )
{
    uint8_t ln = sizeof( midi_channels_cc[chn] );

    for( uint8_t i = 0; i < ln ; i++ ){
        if( midi_channels_cc[chn][i].cc == cc ){
            return midi_channels_cc[chn][i].parameter;
        }
    }
    return -1;
}

void control_change( uint8_t midi_chn, uint8_t data1, uint8_t data2 )
{
    uint8_t chn = assigner_channel_get( midi_chn );
    if( data1 > 49 && data1 < 56 ){
        chn = 5;
    }

    enum parameter_id id = get_cc_parameter( chn, data1 );
    
    if( id < 0 ){
        return;
    }
    struct parameter parameter = parameter_get( id );
    
    //Need to force the range on/off : 0-63/64-127 for BOOL
    uint8_t total_range = 1;

    if( parameter.type == RANGE ){
        total_range = parameter.max - parameter.min;
    }

    total_range += 1;

    uint8_t range_offset = 0;

    if( parameter.min < 0 ){
        range_offset = parameter.min - 1;
    }
    
    *parameter.target = ( (data2 / ( MIDI_MAX_CC / total_range ) ) << 0 ) + range_offset;
}