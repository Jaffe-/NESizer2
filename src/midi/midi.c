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
#include "sysex.h"
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
#include "sequencer/sequencer.h"
#include "patch/patch.h"
#include "settings/settings.h"
#include "note_stack/note_stack.h"

#include "softserial_debug/debug.h"

enum midi_state state = STATE_MESSAGE;

static inline void interpret_message();

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
                    // serial debug test:
                    debug_byte_message(DBG_MIDI_NOTE, 3, midi_channel, msg->data1, msg->data2);
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
            // serial debug test:
            debug_byte_message(DBG_MIDI_CC, 3, midi_channel, msg->data1, msg->data2);
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
                state = STATE_SYSEX; return;
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
