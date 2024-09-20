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
#include "ui/ui_programmer.h"
#include "ui/ui_sequencer.h"
#include "modulation/modulation.h"
#include "io/leds.h"
#include "portamento/portamento.h"
#include "assigner/assigner.h"
#include "sample/sample.h"
#include "sequencer/sequencer.h"
#include "patch/patch.h"
#include "settings/settings.h"
#include "note_stack/note_stack.h"

enum midi_state {
    STATE_MESSAGE,
    STATE_SYSEX,
    STATE_TRANSFER,
    STATE_IGNORE_SYSEX
};

uint8_t midi_notes[5];

static enum midi_state state = STATE_MESSAGE;
static uint8_t sysex_command;

static inline void interpret_message();
static inline void transfer();
static inline void sysex();
static inline void initiate_transfer();
static inline void ignore_sysex();

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
            if (msg->data2 == 0) {
                if (sequencer_midi_note == msg->data1)
                    sequencer_midi_note = 0xFF;
                note_stack_pop(midi_channel, msg->data1);
            } else {
                sequencer_midi_note = msg->data1;
                note_stack_push(midi_channel, msg->data1);
            }
        }
        break;

    case MIDI_CMD_NOTE_OFF:
        if (sequencer_midi_note == msg->data1)
            sequencer_midi_note = 0xFF;
        note_stack_pop(midi_channel, msg->data1);
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
        break;

    case MIDI_CMD_PATCH_CHANGE:
        if (patch_pc_limit(get_patchno_addr(), PATCH_MIN, PATCH_MAX, msg->data1)) {  // range limit
            patch_load(msg->data1);
            settings_write(PROGRAMMER_SELECTED_PATCH, msg->data1);
        }
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
    case STATE_IGNORE_SYSEX:
        ignore_sysex(); break;
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
    if (midi_io_bytes_remaining() >= 8) {

        uint8_t sysex_id = midi_io_read_byte();
        uint8_t device_id = midi_io_read_byte();

        if ((sysex_id == SYSEX_ID) && (device_id == SYSEX_DEVICE_ID)) {

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

        else {
            ignore_sysex();
        }
    }
}

#define ERROR_MIDI_RX_LEN_MISMATCH (1 << 2)

static inline void transfer()
/*
  Handlers transfering of data via MIDI
*/
{
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

static inline void ignore_sysex()
{
    // for handling streams of sysex data not meant for NESizer.
    state = STATE_IGNORE_SYSEX;

    while (midi_io_bytes_remaining() >= 1) {
        uint8_t val = midi_io_read_byte();

        if (val == SYSEX_STOP) {
            state = STATE_MESSAGE;
        }
    }
}
