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



  Low level LED control.

  Provides macros and functions for setting LEDs on the front panel
  board, and a task/task.handler for refreshing them.
*/


#pragma once

#include <stdint.h>

// Helper macros
#define leds_row(BTN) ((BTN) / 8)
#define leds_col(BTN) ((BTN) % 8)

#define leds_on(BTN) (leds[leds_row(BTN)] |= 1 << leds_col(BTN))
#define leds_off(BTN) (leds[leds_row(BTN)] &= ~(1 << leds_col(BTN)))
#define leds_toggle(BTN) (leds[leds_row(BTN)] ^= 1 << leds_col(BTN))

#define LEDS_7SEG_DOT 17
#define LEDS_7SEG_MINUS 18

// Global variables
extern uint8_t leds[5];
extern uint8_t row_mirror;

// Functions
void leds_refresh(void);
void leds_7seg_set(uint8_t row, uint8_t value);
void leds_7seg_two_digit_set(uint8_t row1, uint8_t row2, uint8_t value);
void leds_7seg_two_digit_set_hex(uint8_t rwo1, uint8_t row2, uint8_t value);
void leds_7seg_dot_on(uint8_t row);
void leds_7seg_dot_off(uint8_t row);
void leds_7seg_clear(uint8_t row);
void leds_7seg_note_set(uint8_t row1, uint8_t row2, uint8_t note);
void leds_7seg_minus(uint8_t row);
void leds_7seg_custom(uint8_t row, uint8_t value);
