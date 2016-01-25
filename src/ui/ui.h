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



  General user interface routines

  Contains the UI handler which checks button presses and transfers control to 
  the corresponding mode handlers, and the LED refresh handler.
*/


#pragma once

#include "parameter/parameter.h"
#include "io/leds.h"
#include "io/input.h"

#define BTN_SQ1 0 // 16
#define BTN_SQ2 1 // 17
#define BTN_TRI 2 // 18
#define BTN_NOISE 3 // 19
#define BTN_DMC 4 // 20

#define BTN_SAVE 16 // 0
#define BTN_SET 17 // 1
#define BTN_PAGE1 18 // 2
#define BTN_PAGE2 19 // 3
#define BTN_SEQUENCER 20 // 4
#define BTN_SETTINGS 21 // 5
#define BTN_UP 22 // 6
#define BTN_DOWN 23 // 7

#define button_row(BTN) ((BTN) / 8)
#define button_col(BTN) ((BTN) % 8)

// Checks wether a button is being held down
#define button_on_array(ARRAY, BTN) ((ARRAY[button_row(BTN)] & (1 << button_col(BTN))) != 0)
#define button_on(BTN) (button_on_array(input, BTN))

// Was it just pressed?
#define button_pressed(BTN) (button_on_array(input, BTN) && (!button_on_array(prev_input, BTN)))

// Was it just depressed?
#define button_depressed(BTN) (!button_on_array(input, BTN) && button_on_array(prev_input, BTN))

// Gets the boolean value of a particular button
#define button_getbool(BTN) ((input[button_row(BTN)] >> button_col(BTN)) & 1)

/*
   Functions for handling LED states that are compressed into 2-bit values.
   0 means off, 1 means on and 2 means blink.
*/
#define button_led_byte(BTN) ((BTN) / 4)
#define button_led_shift(BTN) (((BTN) % 4) * 2)
#define button_led_set(BTN, VAL) (button_leds[button_led_byte(BTN)] |= (VAL) << button_led_shift(BTN))
#define button_led_get(BTN) ((button_leds[button_led_byte(BTN)] >> button_led_shift(BTN)) & 0b11)
#define button_led_on(BTN) (button_led_set(BTN, 1))
#define button_led_blink(BTN) button_led_set(BTN, 0b10); leds_on(BTN)
#define button_led_off(BTN) (button_leds[button_led_byte(BTN)] &= ~(0b11 << button_led_shift(BTN)))

// The current mode (PROGRAMMER, PATTERN, ASSIGNER or SETTINGS, or GETVALUE, TRANSFER)

enum mode {
  MODE_PAGE1,
  MODE_PAGE2,
  MODE_SEQUENCER,
  MODE_SETTINGS,
  MODE_GETVALUE,
  MODE_TRANSFER
};

extern enum mode mode;

struct getvalue_config {
  struct parameter parameter;
  uint8_t button1;
  uint8_t button2;
  enum {
    INACTIVE, ACTIVE
  } state;
  enum mode previous_mode;
};

// Previous inputs
extern uint8_t prev_input[3];
extern uint8_t* button_leds;

struct getvalue_config getvalue;

void ui_handler(void);
void ui_leds_handler(void);
uint8_t ui_updown(int8_t* value, int8_t min, int8_t max);

