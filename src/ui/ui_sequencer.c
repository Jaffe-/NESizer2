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



  Sequencer user interface

  Handles the user interface when in sequencer mode.
*/


#include <stdint.h>
#include <avr/pgmspace.h>
#include "ui_sequencer.h"
#include "ui.h"
#include "midi/midi.h"
#include "assigner/assigner.h"
#include "sequencer/sequencer.h"

#define BTN_OCTAVE 6
#define BTN_NOTE_CLEAR 7

#define BTN_RECORD 8
#define BTN_END_POINT 14
#define BTN_SCALE 15

#define BTN_BACK 16
#define BTN_PLAY 16
#define BTN_OK 17

enum state { STATE_TOPLEVEL, STATE_ENTER_NOTE, STATE_SAVE, STATE_SELECT_NOTE, STATE_PLAYING, STATE_RECORDING, STATE_ENTER_END_POINT };

static enum state state = STATE_TOPLEVEL;

static void select_pattern(void);
static void enter_note(void);
static void select_note(void);
static void play_pattern(void);
static void record_pattern(void);
static void enter_end_point(void);

static void enter_note_init(uint8_t);
void enter_end_point_init(void);
static void save_init(uint8_t*);
static void display_pattern(void);

// Used to remember the last settings for the channel
static uint8_t channel_octave[5] = {4, 4, 4, 0, 0};
static uint8_t channel_length[5] = {3, 3, 3, 3, 3};

static uint8_t current_channel;
static uint8_t current_pattern;
static uint8_t current_pos;
static uint8_t current_note;

uint8_t sequencer_leds[6];

void (*state_handlers[])(void) = {
    [STATE_TOPLEVEL] = select_pattern,
    [STATE_SELECT_NOTE] = select_note,
    [STATE_ENTER_NOTE] = enter_note,
    [STATE_SAVE] = select_pattern,
    [STATE_PLAYING] = play_pattern,
    [STATE_RECORDING] = record_pattern,
    [STATE_ENTER_END_POINT] = enter_end_point
};

static inline uint8_t btn_to_note(uint8_t btn);


void sequencer(void)
{
    state_handlers[state]();
}


/*
  State handlers
*/

/* Pattern selection state */
void select_pattern(void)
{

    if (state == STATE_SAVE) {
        sequencer_pattern_save(current_pattern);
        state = STATE_TOPLEVEL;
    }

    else if (ui_updown((int8_t*)&current_pattern, 0, 99))
        sequencer_pattern_load(current_pattern);

    else if (button_pressed(BTN_SAVE)) {
        save_init(&current_pattern);
    }

    for (uint8_t b = 0; b < 5; b++) {
        if (button_pressed(b)) {
            current_channel = b;
            state = STATE_SELECT_NOTE;
        }
    }

    if (button_on(BTN_RECORD)) {
        for (uint8_t chn = 0; chn < 5; chn++) {
            if (button_pressed(chn)) {
                sequencer_record(chn);
                state = STATE_RECORDING;
            }
        }
    }
    if (button_pressed(BTN_PLAY)) {
        sequencer_play();
        state = STATE_PLAYING;
    }

    if (button_pressed(BTN_SCALE)) {
        getvalue.button1 = BTN_SCALE;
        getvalue.button2 = 0xFF;
        getvalue.parameter.target = &sequencer_pattern.scale;
        getvalue.parameter.type = SCALE;
        getvalue.parameter.min = 0;
        getvalue.parameter.max = 2;
        getvalue.previous_mode = mode;
        mode = MODE_GETVALUE;
    }

    if (button_pressed(BTN_END_POINT)) {
        enter_end_point_init();
    }

    leds_7seg_two_digit_set(3, 4, current_pattern);
}


/* Note selection state */

void select_note(void)
{
    // Light up buttons according to data
    display_pattern();

    // Go through each of the 16 upper buttons
    for (uint8_t i = 0; i < 16; i++) {
        if (button_pressed(i)) {
            enter_note_init(i);
            break;
        }
    }

    leds_7seg_set(4, current_channel + 1);

    if (button_pressed(BTN_BACK)) {
        for (uint8_t i = 0; i < 16; i++)
            button_led_off(i);
        state = STATE_TOPLEVEL;
    }
}


/* Note entering state */
void enter_note_init(uint8_t btn)
{
    button_led_blink(btn);
    current_pos = btn;
    sequencer_midi_note = 0xFF;
    current_note = sequencer_pattern.notes[current_channel][current_pos].note;
    state = STATE_ENTER_NOTE;
}

void enter_note_exit(void)
{
    button_led_off(current_pos);
    state = STATE_SELECT_NOTE;
}

void enter_note(void)
{
    uint8_t new_note = 0xFF;

    // Check if any of the note buttons have been pressed:
    for (uint8_t i = 0; i < 16; i++) {
        if (button_pressed(i)) {
            uint8_t note = btn_to_note(i);
            if (note != 0xFF)
                new_note = note;
        }
    }

    // Check for MIDI note:
    if (sequencer_midi_note != 0xFF) {
        new_note = sequencer_midi_note;
        sequencer_midi_note = 0xFF;
    }

    if (new_note != 0xFF) {
        current_note = new_note;
        play_note(current_channel, current_note);
        sequencer_single_note(current_channel);
    }

    // Other button presses:
    if (button_pressed(BTN_NOTE_CLEAR)) {
        sequencer_pattern.notes[current_channel][current_pos].length = 0;
        enter_note_exit();
    }

    else if (button_on(BTN_OCTAVE)) {
        leds_7seg_two_digit_set(3, 4, channel_octave[current_channel]);
        ui_updown((int8_t*)&channel_octave[current_channel], 1, 7);
    }

    else if (button_pressed(BTN_OK)) {
        if (current_note != 0xFF) {
            sequencer_pattern.notes[current_channel][current_pos].note = current_note;
            sequencer_pattern.notes[current_channel][current_pos].length = channel_length[current_channel];
        }
        enter_note_exit();
    }

    else if (button_pressed(BTN_BACK)) {
        enter_note_exit();
    }

    else {
        leds_7seg_two_digit_set(3, 4, channel_length[current_channel]);
        ui_updown((int8_t*)&channel_length[current_channel], 1, 4);
    }

}

void enter_end_point_init(void)
{
    for (uint8_t i = 0; i < sequencer_pattern.end_point; i++) {
        button_led_blink(i);
    }
    state = STATE_ENTER_END_POINT;
}

void enter_end_point_exit(void)
{
    for (uint8_t i = 0; i < 16; i++) {
        button_led_off(i);
    }
    state = STATE_TOPLEVEL;
}

void enter_end_point(void)
{
    for (uint8_t i = 0; i < 16; i++) {
        if (button_pressed(i)) {
            sequencer_pattern.end_point = i + 1;
            enter_end_point_exit();
        }
    }
}

static void play_pattern(void)
{
    for (uint8_t i = 0; i < 16; i++)
        button_led_off(i);
    leds_7seg_two_digit_set(3, 4, sequencer_tempo_count);

    ui_updown((int8_t*)&sequencer_tempo_count, 1, 99);

    if (button_pressed(BTN_PLAY)) {
        sequencer_stop();
        state = STATE_TOPLEVEL;
    }
    else
        button_led_on(sequencer_cur_position);
}

static void record_pattern(void)
{
    play_pattern();
}

static void display_pattern(void)
{
    for (uint8_t i = 0; i < 16; i++) {
        if (sequencer_pattern.notes[current_channel][i].length != 0)
            button_led_on(i);
        else
            button_led_off(i);
    }
}

void save_init(uint8_t* pattern)
{
    getvalue.button1 = BTN_SAVE;
    getvalue.button2 = 0xFF;
    getvalue.parameter.target = pattern;
    getvalue.parameter.type = RANGE;
    getvalue.parameter.min = 0;
    getvalue.parameter.max = 99;
    getvalue.previous_mode = mode;
    mode = MODE_GETVALUE;
    state = STATE_SAVE;
}

static inline uint8_t btn_to_note(uint8_t btn)
{
    static uint8_t notes[16] = {
        1, 3, 0xFF, 6, 8, 10, 0xFF, 0xFF, 0, 2, 4, 5, 7, 9, 11, 12
    };

    uint8_t note = notes[btn];
    if (note != 0xFF) {
        // Add octave setting to the note:
        note += 12 * (channel_octave[current_channel] + 1);
    }

    return note;
}
