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



  Channel note assignment

  Assigns notes from MIDI or button input to sound channels.
*/

#include <stdbool.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "modulation/modulation.h"
#include "portamento/portamento.h"
#include "apu/apu.h"
#include "envelope/envelope.h"
#include "assigner/assigner.h"
#include "sample/sample.h"
#include "modulation/periods.h"
#include "midi/midi.h"
#include "settings/settings.h"

enum assigner_mode assigner_lower_mode = MONO;
enum assigner_mode assigner_upper_mode = MONO;
bool assigner_split;
int8_t assigner_split_point;
int8_t assigner_upper_mask[5];

int8_t assigner_enabled[5];

uint8_t assigned_notes[5];

struct group {
    uint8_t midi_channel;
    uint8_t last_assigned_lower;
    uint8_t last_assigned_upper;
    uint8_t members;
};

static struct group groups[5];

/* Internal group functions */
static inline void add_member(int8_t group, uint8_t chn);
static inline void remove_member(int8_t group, uint8_t chn);
static inline bool has_member(int8_t group, uint8_t channel);
static inline bool empty(int8_t group);
static inline int8_t find_group(uint8_t midi_channel);
static inline int8_t new_group(uint8_t midi_channel);
static inline void group_notify_note_on(int8_t group, uint8_t note);
static inline void group_notify_note_off(int8_t group, uint8_t note);

uint8_t midi_channels[5];

void assigner_setup(void)
{
    for (uint8_t i = 0; i < 5; i++)
        assigner_midi_channel_change(settings_read(MIDI_CHN + i), i);

    assigner_lower_mode = settings_read(ASSIGNER_LOWER_MODE);
    assigner_upper_mode = settings_read(ASSIGNER_UPPER_MODE);
    assigner_split = settings_read(ASSIGNER_SPLIT);
}

/* This is called by the SETTINGS UI when the user assigns
   the given channel to a given midi channel */
void assigner_midi_channel_change(uint8_t midi_channel, uint8_t chn)
{
    settings_write(MIDI_CHN + chn, midi_channel);

    // If new channel is different from old, remove chn as a member of its group
    if (midi_channels[chn] != midi_channel && midi_channels[chn] != 0) {
        // Avoid stuck notes
        stop_note(chn);

        int8_t old_group = find_group(midi_channels[chn]);
        if (old_group != -1)
            remove_member(old_group, chn);
        else while(1); // should never happen!
    }

    midi_channels[chn] = midi_channel;

    // 0 means no midi channel, so there is nothing more to do
    if (midi_channel == 0)
        return;

    // See if there is a group already listening to the given channel
    int8_t group = find_group(midi_channel);
    if (group == -1) {
        group = new_group(midi_channel);
    }
    add_member(group, chn);
}

uint8_t assigner_midi_channel_get(uint8_t chn)
{
    return midi_channels[chn];
}

void assigner_notify_note_on(uint8_t midi_channel, uint8_t note)
{
    /* find the group listening on the channel */
    int8_t group;
    if ((group = find_group(midi_channel)) != -1)
        group_notify_note_on(group, note);
}

void assigner_notify_note_off(uint8_t midi_channel, uint8_t note)
{
    /* find the group listening on the channel */
    uint8_t group;
    if ((group = find_group(midi_channel)) != -1)
        group_notify_note_off(group, note);
}


static inline int8_t new_group(uint8_t midi_channel)
{
    for (uint8_t group = 0; group < 5; group++) {
        if (empty(group)) {
            groups[group].midi_channel = midi_channel;
            groups[group].last_assigned_lower = 0;
            groups[group].last_assigned_upper = 0;
            groups[group].members = 0;
            return group;
        }
    }

    // We should never arrive here!
    while (1);
}

static inline void add_member(int8_t group, uint8_t chn)
{
    groups[group].members |= (1 << chn);
}

static inline void remove_member(int8_t group, uint8_t chn)
{
    groups[group].members &= ~(1 << chn);
}

static inline bool has_member(int8_t group, uint8_t channel)
{
    return groups[group].members & (1 << channel);
}

static inline bool empty(int8_t group)
{
    return groups[group].members == 0;
}

static inline int8_t find_group(uint8_t midi_channel)
{
    for (int8_t i = 0; i < 5; i++) {
        if (groups[i].midi_channel == midi_channel) {
            return i;
        }
    }
    return -1;
}

static inline void group_notify_note_on(int8_t group, uint8_t note)
{
    bool is_upper = note >= assigner_split_point;
    if ((!assigner_split && assigner_upper_mode == MONO) ||
        (is_upper && assigner_upper_mode == MONO) ||
        (!is_upper && assigner_lower_mode == MONO)) {
        for (uint8_t chn = 0; chn < 5; chn++) {
            if (!has_member(group, chn))
                continue;

            if (!assigner_split ||
                (is_upper && (assigner_upper_mask[chn])) ||
                (!is_upper && !(assigner_upper_mask[chn]))) {
                stop_note(chn);
                play_note(chn, note);
            }
        }
    }

    else {
        // Find next to take note
        uint8_t chn = 0xFF;
        uint8_t lowest = 0xFF;
        for (uint8_t i = 0; i < 5; i++) {
            if (!has_member(group, i))
                continue;

            if (assigner_split &&
                ((is_upper && !(assigner_upper_mask[i])) ||
                 (!is_upper && (assigner_upper_mask[i]))))
                continue;

            if (lowest == 0xFF)
                lowest = i;

            if (!assigned_notes[i]) {
                chn = i;
                break;
            }
        }

        // If we found no lowest channel, there are no channels
        // assigned to this half of the MIDI keyboard
        if (lowest == 0xFF)
            return;

        // Take the lowest found if all have assigned notes
        if (chn == 0xFF) {
            chn = lowest;
            stop_note(chn);
        }
        play_note(chn, note);
    }
}

static inline void group_notify_note_off(int8_t group, uint8_t note)
{
    for (uint8_t chn = 0; chn < 5; chn++) {
        if (has_member(group, chn)) {
            if (assigned_notes[chn] == note)
                stop_note(chn);
        }
    }
}

int8_t midi_note_to_note(uint8_t midi_note)
{
    return (int8_t)midi_note - 24;
}

void play_note(uint8_t channel, uint8_t midi_note)
{
    if (!assigner_enabled[channel])
        return;

    uint8_t note = midi_note_to_note(midi_note);
    assigned_notes[channel] = midi_note;

    switch (channel) {
    case CHN_SQ1:
        env[0].gate = 1;
        portamento_target_notes[0] = note;
        break;

    case CHN_SQ2:
        env[1].gate = 1;
        portamento_target_notes[1] = note;
        break;

    case CHN_TRI:
        tri.silenced = 0;
        portamento_target_notes[2] = note;
        break;

    case CHN_NOISE:
        env[2].gate = 1;
        noise_period = note - 24;
        break;

    case CHN_DMC:
        if (sample_occupied(midi_note - 60)) {
            sample_load(&dmc.sample, midi_note - 60);
            if (dmc.sample.size != 0)
                dmc.sample_enabled = 1;
            break;
        }
    }
}

void stop_note(uint8_t channel)
{
    switch (channel) {
    case CHN_SQ1:
        env[0].gate = 0;
        break;

    case CHN_SQ2:
        env[1].gate = 0;
        break;

    case CHN_TRI:
        tri.silenced = 1;
        break;

    case CHN_NOISE:
        env[2].gate = 0;
        break;

    case CHN_DMC:
        dmc.sample_enabled = 0;
    }

    assigned_notes[channel] = 0;
}
