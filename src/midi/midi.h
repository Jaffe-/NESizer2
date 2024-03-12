/*
  Copyright 2014-2015 Johan Fjeldtvedt

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


#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "parameter/parameter.h"

#define MIDI_NOTE_LIST_MAX 8
#define MIDI_MAX_CC 0x80 //128
#define MIDI_MID_CC 0x3F //63
// #define NULL ( (void *) 0)

enum sysex_cmd {
    SYSEX_CMD_SAMPLE_LOAD = 1,
    SYSEX_CMD_SETTINGS_LOAD,
    SYSEX_CMD_PATCH_LOAD,
    SYSEX_CMD_SEQUENCE_LOAD,
};

enum sysex_data_format {
    SYSEX_DATA_FORMAT_4BIT,
    SYSEX_DATA_FORMAT_7BIT_TRUNC,
};

struct midi_channel {
    uint8_t note_list[MIDI_NOTE_LIST_MAX];
    uint8_t note_list_length;
    uint8_t channel;
    uint8_t listeners_count;
    uint8_t listeners;
};

extern uint8_t midi_transfer_progress;
extern uint8_t midi_notes[5];

void midi_channel_subscribe(uint8_t midi_chn, uint8_t chn);
void midi_channel_unsubscribe(uint8_t midi_chn, uint8_t chn);
void midi_handler(void);

struct midi_command{
    uint8_t cc;
    enum parameter_id parameter;
    uint8_t cc_toggle;
    int8_t* stashed_state; //Saved paramter target value
    bool* stash_active; //Is the state stored
};
struct state_toggle{
    int8_t state;
    bool stashed;
};
void midi_init(void);
void control_change(uint8_t midi_chn, uint8_t data1, uint8_t data2);
int8_t midi_command_get_cc(uint8_t chn, uint8_t data1 );
struct midi_command midi_command_get(uint8_t chn, int8_t index );
uint8_t get_target_value(struct parameter parameter, int8_t cc_value );
void state_toggle_init (struct state_toggle *arg, size_t ln );