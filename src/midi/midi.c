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

typedef enum {STATE_MESSAGE, STATE_SYSEX, STATE_TRANSFER} midi_state_e;

uint8_t midi_notes[5];

static midi_state_e state = STATE_MESSAGE;
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
        if (getvalue.state == ACTIVE &&
            getvalue.parameter.type == NOTE)
            getvalue.midi_note = msg->data1;
        else {
            sequencer_midi_note = msg->data1;
            assigner_notify_note_on(midi_channel, msg->data1);
        }
        break;

    case MIDI_CMD_NOTE_OFF:
        if (sequencer_midi_note == msg->data1)
            sequencer_midi_note == 0xFF;
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
        if (midi_io_read_message(&msg)) {

            if (midi_is_channel_message(msg.command)) {
                midi_channel_apply(&msg);
            }

            else {
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
    }
}

#define SYSEX_STOP 0xF7
#define SYSEX_CMD_SAMPLE_LOAD 1
#define SYSEX_CMD_FIRMWARE_LOAD 2

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

        else if (sysex_command == SYSEX_CMD_FIRMWARE_LOAD) {
            // To do
            state = STATE_TRANSFER;
        }
    }
}

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
            mode = MODE_PAGE1;
            state = STATE_MESSAGE;
        }

        else if ((val & 0x80) == 0) {
            if (sysex_command == SYSEX_CMD_FIRMWARE_LOAD) {
                if (nibble_flag == 0)
                    temp_val = val << 4;
                else {
                    temp_val |= val;
                    // TODO: Write data to firmware memory
                }
                nibble_flag ^= 1;
            }

            else if (sysex_command == SYSEX_CMD_SAMPLE_LOAD) {
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
    mode = MODE_TRANSFER;

    // Disable DMC
    dmc.sample_enabled = 0;

    state = STATE_TRANSFER;
    midi_transfer_progress = 0;
}
