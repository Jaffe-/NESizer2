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



  Sequencer

*/

#include <stdint.h>
#include <stdbool.h>
#include "sequencer.h"
#include "assigner/assigner.h"
#include "io/memory.h"

#define SEQUENCER_START 6656
#define PATTERN_SIZE 162 // 2 * 5 * 16 + 2

#define ENTER_NOTE_COUNT 100

struct sequencer_pattern sequencer_pattern;

uint8_t sequencer_tempo_count = 10;
uint8_t sequencer_cur_position;
bool sequencer_ext_clock;
uint8_t sequencer_midi_note;

static uint8_t duration_counter;
static uint8_t tempo_counter;
static uint8_t midi_clock_count;
static uint8_t record_chn;

static enum { SINGLE_NOTE, PLAY, RECORD, STOP } mode = STOP;

uint8_t enter_note_chn;

void tick(void);

struct memory_context ctx;

void sequencer_handler(void)
{
    if (mode == SINGLE_NOTE && ++tempo_counter == ENTER_NOTE_COUNT) {
        stop_note(enter_note_chn);
        mode = STOP;
    }

    if (sequencer_ext_clock || (mode != PLAY && mode != RECORD))
        return;

    if (++tempo_counter == sequencer_tempo_count) {
        tempo_counter = 0;
        tick();
    }
}

void sequencer_single_note(uint8_t chn)
{
    enter_note_chn = chn;
    mode = SINGLE_NOTE;
}

void sequencer_midi_clock(void)
{
    if (!sequencer_ext_clock || (mode != PLAY && mode != RECORD))
        return;

    if (++midi_clock_count == (1 << sequencer_pattern.scale)) {
        midi_clock_count = 0;
        tick();
    }
}

void sequencer_play(void)
{
    mode = PLAY;
    sequencer_cur_position = 0;
}

void sequencer_record(uint8_t chn)
{
    mode = RECORD;
    record_chn = chn;
    sequencer_cur_position = 0;
    sequencer_midi_note = 0xFF;
}

void sequencer_stop(void)
{
    mode = STOP;
    tempo_counter = 0;
    duration_counter = 0;
    midi_clock_count = 0;
    for (uint8_t chn = 0; chn < 5; chn++) {
        const struct sequencer_note* current_note = &sequencer_pattern.notes[chn][sequencer_cur_position];
        if (current_note->length > 0)
            stop_note(chn);
    }
}

void sequencer_continue(void)
{
    mode = PLAY;
}

void sequencer_pattern_load(uint8_t pattern)
{
    memory_set_address(&ctx, SEQUENCER_START + PATTERN_SIZE * pattern);
    for (uint8_t chn = 0; chn < 5; chn++) {
        for (uint8_t i = 0; i < 16; i++) {
            sequencer_pattern.notes[chn][i].note = memory_read_sequential(&ctx);
            sequencer_pattern.notes[chn][i].length = memory_read_sequential(&ctx);
        }
    }
    sequencer_pattern.scale = memory_read_sequential(&ctx);
    sequencer_pattern.end_point = memory_read_sequential(&ctx);
}

void sequencer_pattern_save(uint8_t pattern)
{
    memory_set_address(&ctx, SEQUENCER_START + PATTERN_SIZE * pattern);
    for (uint8_t chn = 0; chn < 5; chn++) {
        for (uint8_t i = 0; i < 16; i++) {
            memory_write_sequential(&ctx, sequencer_pattern.notes[chn][i].note);
            memory_write_sequential(&ctx, sequencer_pattern.notes[chn][i].length);
        }
    }
    memory_write_sequential(&ctx, sequencer_pattern.scale);
    memory_write_sequential(&ctx, sequencer_pattern.end_point);
}

void sequencer_pattern_init()
{
    memory_set_address(&ctx, SEQUENCER_START);
    for (uint8_t pat = 0; pat < 100; pat++) {
        for (uint8_t chn = 0; chn < 5; chn++) {
            for (uint8_t i = 0; i < 16; i++) {
                memory_write_sequential(&ctx, 0);
                memory_write_sequential(&ctx, 0);
            }
        }
        memory_write_sequential(&ctx, 2); // scale
        memory_write_sequential(&ctx, 16); // end point
    }
}

void tick(void)
{
    for (uint8_t chn = 0; chn < 5; chn++) {
        struct sequencer_note* current_note = &sequencer_pattern.notes[chn][sequencer_cur_position];

        if (mode == RECORD && record_chn == chn && sequencer_midi_note != 0xFF) {
            current_note->note = sequencer_midi_note;
            current_note->length = 4; // actually time this
            sequencer_midi_note = 0xFF;
        }

        if (duration_counter == 0) {
            if (current_note->length > 0)
                play_note(chn, current_note->note);
        }
        else if (current_note->length == duration_counter) {
            stop_note(chn);
        }

    }

    if (++duration_counter == 6) {
        duration_counter = 0;
        if (++sequencer_cur_position == sequencer_pattern.end_point)
            sequencer_cur_position = 0;
    }
}
