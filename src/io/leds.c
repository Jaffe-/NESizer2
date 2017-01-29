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


#include <stdint.h>
#include "io/leds.h"
#include "io/bus.h"

#define isone(VAL) ((VAL) != 0)

uint8_t leds[5];
uint8_t row_mirror;

#define SYM_0 0b11111100
#define SYM_1 0b01100000
#define SYM_2 0b11011010
#define SYM_3 0b11110010
#define SYM_4 0b01100110
#define SYM_5 0b10110110
#define SYM_6 0b10111110
#define SYM_7 0b11100000
#define SYM_8 0b11111110
#define SYM_9 0b11110110
#define SYM_A 0b11101110
#define SYM_b 0b00111110
#define SYM_C 0b10011100
#define SYM_d 0b01111010
#define SYM_E 0b10011110
#define SYM_F 0b10001110
#define SYM_G 0b10111100
#define SYM_DOT 0b00000001
#define SYM_MINUS 0b00000010

static const uint8_t leds_7seg_values[19] = {
    SYM_0,
    SYM_1,
    SYM_2,
    SYM_3,
    SYM_4,
    SYM_5,
    SYM_6,
    SYM_7,
    SYM_8,
    SYM_9,
    SYM_A,
    SYM_b,
    SYM_C,
    SYM_d,
    SYM_E,
    SYM_F
};

void leds_refresh(void)
/* This function is intended to be called by the task/task.handler
   at a given frequency. Each time it is called, a new LED column
   is displayed.
*/
{
    static uint8_t current_col = 1;
    static uint8_t current_row_half = 0;

    // Fetch LED status for the rows in the current column
    if (current_row_half == 0) {
        row_mirror = 0x1F & ~(isone(leds[0] & current_col)
                              | (isone(leds[1] & current_col) << 1)
                              | (isone(leds[2] & current_col) << 2));
    }
    else {
        row_mirror = 0x1F & ~((isone(leds[3] & current_col) << 3)
                              | (isone(leds[4] & current_col) << 4));
    }

    // Address the row latch and put on bus
    bus_select(ROW_ADDRESS);

    bus_write(row_mirror);

    bus_deselect();

    // Switch to column latch, the row value is latched when this happens
    bus_select(LEDCOL_ADDRESS);

    // Activate desired column
    bus_write(current_col);

    // Deselect to latch the value
    bus_deselect();

    if (current_row_half == 1)
        current_col = (current_col == 0x80) ? 1 : current_col << 1;
    current_row_half ^= 1;
}

void leds_7seg_set(uint8_t row, uint8_t val)
{
    leds[row] = leds_7seg_values[val];
}

void leds_7seg_minus(void)
{
    leds[3] |= SYM_MINUS;
}

void leds_7seg_dot_on(uint8_t row)
{
    leds[row] |= SYM_DOT;
}

void leds_7seg_dot_off(uint8_t row)
{
    leds[row] &= ~SYM_DOT;
}

void leds_7seg_clear(uint8_t row)
{
    leds[row] = 0;
}

void leds_7seg_two_digit_set(uint8_t row1, uint8_t row2, uint8_t value)
{
    leds_7seg_set(row2, value % 10);
    leds_7seg_set(row1, value / 10);
}

#define LEDS_7SEG_A 10
#define LEDS_7SEG_B 11
#define LEDS_7SEG_C 12
#define LEDS_7SEG_D 13
#define LEDS_7SEG_E 14
#define LEDS_7SEG_F 15
#define LEDS_7SEG_G 16

void leds_7seg_note_set(uint8_t row1, uint8_t row2, uint8_t note)
{
    static const uint8_t note_symbols[] = {
        SYM_C,
        SYM_d | SYM_DOT,
        SYM_d,
        SYM_E | SYM_DOT,
        SYM_E,
        SYM_F,
        SYM_G | SYM_DOT,
        SYM_G,
        SYM_A | SYM_DOT,
        SYM_A,
        SYM_b | SYM_DOT,
        SYM_b
    };

    leds[row1] = note_symbols[note % 12];
    leds_7seg_set(row2, note / 12 - 1);
}

// For debugging use
void leds_7seg_two_digit_set_hex(uint8_t row1, uint8_t row2, uint8_t value)
{
    leds_7seg_set(row2, value % 16);
    leds_7seg_set(row1, value / 16);
}
