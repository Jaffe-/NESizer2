/*
  Copyright 2024 Johan Fjeldtvedt and Beau Sterling

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



  Note Stack

  Buffers held notes in monophonic mode, to improve live performace experience.
  This behavior is called "legato" mode in most synths.
  In polyphonic mode, reverts to previous behavior.
*/


#include <stdint.h>
#include "apu/apu.h"
#include "assigner/assigner.h"


#define MAX_POLY 8

static int active_notes[16] = { 0 };  // keep track of notes held in each midi channel
static uint8_t note_buffer[16][MAX_POLY];


void clear_note_buffer()
{
    for (int c = 0; c < 16; ++c) {
        active_notes[c] = 0;
        for (int n = 0; n < MAX_POLY; ++n) {
            note_buffer[c][n] = 255;
        }
    }
}


void push_note(uint8_t channel, uint8_t note)
{
    uint8_t c = channel - 1;

    // buffering causes stuck note issues in polyphonic and split mode. also DMC channel behaved better before.
    // revert to previous behaviour as a workaround. this could be handled better:
    if (assigner_split || assigner_upper_mode == POLY || assigner_midi_channel_get(CHN_DMC) == channel) {
        assigner_notify_note_on(channel, note);
        if (active_notes[c] > 0)
            clear_note_buffer();
        else
            return;
    }

    if (active_notes[c] < MAX_POLY) {
        for (int n = 0; n < active_notes[c]; ++n) {
            if (note_buffer[c][n] == note) {
                assigner_notify_note_on(channel, note);
                return;  // if note is already in the buffer, retrigger but don't change the buffer
            }
        }

        assigner_notify_note_on(channel, note);

        note_buffer[c][active_notes[c]] = note;
        active_notes[c]++;
    }
}


void pop_note(uint8_t channel, uint8_t note)
{
    uint8_t c = channel - 1;

    // buffering causes stuck note issues in polyphonic and split mode. also DMC channel behaved better before.
    // revert to previous behaviour as a workaround. this could be handled better:
    if (assigner_split || assigner_upper_mode == POLY || assigner_midi_channel_get(CHN_DMC) == channel) {
        assigner_notify_note_off(channel, note);
        if (active_notes[c] > 0)
            clear_note_buffer();
        else
            return;
    }

    int note_off_match = 0;
    for (int n = 0; n < active_notes[c]; ++n) {  // check buffer to see if note is active
        if (note_buffer[c][n] == note) {
            note_off_match = 1;
            if (n < (active_notes[c]-1)) {  // if note is removed from buffer, shift remaining notes down to prevent empty slots
                note_buffer[c][n] = note_buffer[c][n+1];
                note_buffer[c][n+1] = note;
            }
        }
    }

    if (note_off_match) {
        note_off_match = 0;
        active_notes[c]--;

        note_buffer[c][active_notes[c]] = 255;  // remove requested note and play next note in buffer

        if (active_notes[c])
            assigner_notify_note_on(channel, note_buffer[c][active_notes[c]-1]);
        else
            assigner_notify_note_off(channel, note);
    }
}
