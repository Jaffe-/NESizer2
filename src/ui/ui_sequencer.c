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
#define BTN_BACK 16
#define BTN_PLAY 16
#define BTN_OK 17

enum state { STATE_TOPLEVEL, STATE_ENTER_NOTE, STATE_SAVE, STATE_SELECT_NOTE, STATE_PLAYING };

static enum state state = STATE_TOPLEVEL;

static void select_pattern(void);
static void enter_note(void);
static void select_note(void);
static void play_pattern(void);

static void enter_note_init(uint8_t);
static void save_init(uint8_t*);
static void display_pattern(void);

uint8_t sequencer_midi_note;

// Used to remember the last settings for the channel
static uint8_t channel_octave[5];
static uint8_t channel_length[5];

static uint8_t current_channel;
static uint8_t current_pattern;
static uint8_t current_note;

uint8_t sequencer_leds[6];

void (*state_handlers[])(void) = {
    [STATE_TOPLEVEL] = select_pattern,
    [STATE_SELECT_NOTE] = select_note,
    [STATE_ENTER_NOTE] = enter_note,
    [STATE_SAVE] = select_pattern,
    [STATE_PLAYING] = play_pattern
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

    if (button_pressed(BTN_PLAY)) {
        sequencer_play();
        state = STATE_PLAYING;
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
    current_note = btn;
    sequencer_midi_note = 0xFF;
    state = STATE_ENTER_NOTE;
}

void enter_note_exit(void)
{
    button_led_off(current_note);
    state = STATE_SELECT_NOTE;
}

void enter_note(void)
{
    // Check if any of the note buttons have been pressed:
    uint8_t note = 0xFF;
    for (uint8_t i = 0; i < 16; i++) {
        if (button_pressed(i))
            note = btn_to_note(i);
    }

    // If none of the on-board keys were pressed, check MIDI:
    if (sequencer_midi_note != 0xFF)
        note = sequencer_midi_note;

    if (note != 0xFF) {
        pattern_data[current_channel][current_note].note = note;
        pattern_data[current_channel][current_note].length = channel_length[current_channel];
        enter_note_exit();
    }

    // Other button presses:
    else if (button_pressed(BTN_NOTE_CLEAR)) {
        pattern_data[current_channel][current_note].length = 0;
        enter_note_exit();
    }

    else if (button_on(BTN_OCTAVE)) {
        leds_7seg_two_digit_set(3, 4, channel_octave[current_channel]);
        ui_updown((int8_t*)&channel_octave[current_channel], 1, 7);
    }

    else {
        leds_7seg_two_digit_set(3, 4, channel_length[current_channel]);
        ui_updown((int8_t*)&channel_length[current_channel], 1, 4);
    }

}

static void play_pattern(void)
{
    for (uint8_t i = 0; i < 16; i++)
        button_led_off(i);

    if (button_pressed(BTN_PLAY)) {
        sequencer_stop();
        state = STATE_TOPLEVEL;
    }
    else
        button_led_on(sequencer_cur_position);
}

static void display_pattern(void)
{
    for (uint8_t i = 0; i < 16; i++) {
        if (pattern_data[current_channel][i].length != 0)
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
