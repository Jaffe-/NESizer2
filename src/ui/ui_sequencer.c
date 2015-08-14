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



  Sequencer user interface

  Handles the user interface when in sequencer mode.
*/


#include <stdint.h>
#include <avr/pgmspace.h>
#include "io/leds.h"
#include "ui/ui_sequencer.h"
#include "ui/ui.h"
#include "io/input.h"
#include "midi/midi.h"
#include "assigner/assigner.h"

#define BTN_SHIFT 16
#define BTN_OCTAVE 6
#define BTN_CLEAR 7
#define BTN_OK 17

enum state { STATE_TOPLEVEL, STATE_ENTER_NOTE, STATE_SAVE, STATE_SELECT_NOTE };

static enum state state = STATE_TOPLEVEL;

static void select_pattern(void);
static void enter_note(void);
static void select_note(void);

static void enter_note_init(uint8_t);
static void save_init(uint8_t*);
static void display_pattern(void);

struct pattern_note {
  uint8_t note;
  uint8_t length;
};

static struct pattern_note pattern_data[5][16];

// Used to remember the last settings for the channel
static uint8_t channel_octave[5];
static uint8_t channel_length[5];

static uint8_t current_channel;
static uint8_t current_page;
static uint8_t current_pattern;
static uint8_t current_note;

uint8_t sequencer_leds[24];

void (*state_handlers[])(void) = {
  [STATE_TOPLEVEL] = select_pattern,
  [STATE_SELECT_NOTE] = select_note,
  [STATE_ENTER_NOTE] = enter_note,
  [STATE_SAVE] = select_pattern
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
    pattern_save(current_pattern);
    state = STATE_TOPLEVEL;
  }
  
  else if (ui_updown(&current_pattern, 0, 99))
    pattern_load(current_pattern);
  
  else if (button_pressed(BTN_SAVE)) {
    save_init(&current_pattern);
  }

  else if (button_pressed(BTN_SHIFT))
    state = STATE_SELECT_NOTE;
  
  leds_7seg_two_digit_set(3, 4, current_pattern);

}


/* Note selection state */

void select_note(void)
{
  // Go through each of the 16 upper buttons
  for (uint8_t i = 0; i < 16; i++) {
    if (button_pressed(i)) {
      enter_note_init(i);
      break;
    }
  }
  
  // Light up buttons according to data
  display_pattern();
  
  if (button_on(BTN_SHIFT)) {
    if (ui_updown(&current_page, 0, 9)) 
      pattern_load_data(current_pattern, current_page);
  }
  else {
    if (ui_updown(&current_channel, 1, 5))
      current_channel--;
  }
  
  leds_7seg_set(3, current_page);
  leds_7seg_set(4, current_channel);
  leds_7seg_dot_on(3);
}


/* Note entering state */
void enter_note_init(uint8_t btn)
{
  button_led_blink(btn);
  current_note = btn;
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
  if (note == 0xFF) {  
    for (uint8_t i = 0; i < 5; i++) {
      if (midi_notes[i] != 0)
	note = midi_notes[i];
    }    
  }
  
  if (note != 0xFF) {
    pattern_data[current_channel][current_note].note = note;
    enter_note_exit();
//    state = STATE_SELECT_NOTE;
  }

  // Other button presses:
  else if (button_pressed(BTN_CLEAR)) {
    pattern_data[current_channel][current_note].length = 0;
    //  state = STATE_SELECT_NOTE;
    enter_note_exit();
  }
    
  else if (button_on(BTN_OCTAVE)) {
    leds_7seg_two_digit_set(3, 4, channel_octave[current_channel]);
    ui_updown(&channel_octave[current_channel], 1, 7);
  }

  else {
    leds_7seg_two_digit_set(3, 4, channel_length[current_channel]);
    if (ui_updown(&channel_length[current_channel], 1, 4))
      pattern_data[current_channel][current_note].length = channel_length[current_channel];
  }

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

void sequence_handler(void)
{
  
}

void pattern_load_data(uint8_t a, uint8_t b)
{

}

void pattern_load(uint8_t a)
{

}

void pattern_save(uint8_t a)
{

}

void save_init(uint8_t* pattern)
{
  ui_getvalue_session.button1 = BTN_SAVE;
  ui_getvalue_session.button2 = 0xFF;
  ui_getvalue_session.parameter.target = pattern;
  ui_getvalue_session.parameter.type = RANGE;
  ui_getvalue_session.parameter.min = 0;
  ui_getvalue_session.parameter.max = 99;
  mode |= MODE_GETVALUE;
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
