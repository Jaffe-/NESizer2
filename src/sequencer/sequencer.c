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
#define PATTERN_SIZE 160 // 2 * 5 * 16

struct sequencer_note pattern_data[5][16];

uint8_t sequencer_tempo_count = 10;
uint8_t sequencer_cur_position;

static uint8_t duration_counter;
static uint8_t tempo_counter;
static bool play;

struct memory_context ctx;

void sequencer_handler(void)
{
    if (!play)
        return;

    if (++tempo_counter == sequencer_tempo_count) {
        tempo_counter = 0;

        for (uint8_t chn = 0; chn < 5; chn++) {
            const struct sequencer_note* current_note = &pattern_data[chn][sequencer_cur_position];

            if (duration_counter == 0) {
                if (current_note->length > 0)
                    play_note(chn, current_note->note);
            }
            else if (current_note->length == duration_counter) {
                stop_note(chn);
            }
        }

        if (++duration_counter == 4) {
            duration_counter = 0;
            sequencer_cur_position = (sequencer_cur_position + 1) % 16;
        }
    }
}

void sequencer_play(void)
{
    play = true;
    sequencer_cur_position = 0;
    tempo_counter = 0;
    duration_counter = 0;
}

void sequencer_stop(void)
{
    play = false;
    for (uint8_t chn = 0; chn < 5; chn++) {
        const struct sequencer_note* current_note = &pattern_data[chn][sequencer_cur_position];
        if (current_note->length > 0)
            stop_note(chn);
    }
}

void sequencer_pattern_load(uint8_t pattern)
{
    memory_set_address(&ctx, SEQUENCER_START + PATTERN_SIZE * pattern);
    for (uint8_t chn = 0; chn < 5; chn++) {
        for (uint8_t i = 0; i < 16; i++) {
            pattern_data[chn][i].note = memory_read_sequential(&ctx);
            pattern_data[chn][i].length = memory_read_sequential(&ctx);
        }
    }
}

void sequencer_pattern_save(uint8_t pattern)
{
    memory_set_address(&ctx, SEQUENCER_START + PATTERN_SIZE * pattern);
    for (uint8_t chn = 0; chn < 5; chn++) {
        for (uint8_t i = 0; i < 16; i++) {
            memory_write_sequential(&ctx, pattern_data[chn][i].note);
            memory_write_sequential(&ctx, pattern_data[chn][i].length);
        }
    }
}
