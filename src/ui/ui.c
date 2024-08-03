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



  General user interface routines

  Contains the UI handler which checks button presses and transfers control to
  the corresponding mode handlers, and the LED refresh handler.
*/


#include "ui/ui.h"
#include "ui/ui_sequencer.h"
#include "ui/ui_programmer.h"
#include "ui/ui_assigner.h"
#include "ui/ui_settings.h"
#include "io/leds.h"
#include "io/input.h"
#include "io/battery.h"
#include "midi/midi.h"
#include "modulation/modulation.h"
#include "patch/patch.h"
#include "io/memory.h"
#include "assigner/assigner.h"

#include <stdbool.h>

#define BLINK_CNT 30

enum mode mode = MODE_SILENCE;

uint8_t prev_input[3] = {0};

/* Pointer to array holding LED states for chosen mode */
uint8_t* button_leds = programmer_leds;

static inline uint8_t remove_flags(uint8_t mode)
{
    return mode & 0x0F;
}

struct mode_data {
    uint8_t button;
    uint8_t* leds;
    void (*handler)(void);
};

struct getvalue_config getvalue;

static void getvalue_handler(void);
static void show_startup_errors(void);
static void silence_channels(void);
static void transfer(void);
static void error_handler(void);

struct mode_data modes[] = {
    [MODE_PAGE1] = {.button = BTN_PAGE1,
                    .leds = programmer_leds,
                    .handler = programmer},
    [MODE_PAGE2] = {.button = BTN_PAGE2,
                    .leds = programmer_leds,
                    .handler = programmer},
    [MODE_SEQUENCER] = {.button = BTN_SEQUENCER,
                        .leds = sequencer_leds,
                        .handler = sequencer},
    [MODE_SETTINGS] = {.button = BTN_SETTINGS,
                       .leds = settings_leds,
                       .handler = settings},
    [MODE_GETVALUE] = {.button = 0xFF,
                       .leds = 0,
                       .handler = getvalue_handler},
    [MODE_TRANSFER] = {.button = 0xFF,
                       .leds = 0,
                       .handler = transfer},
    [MODE_ERROR] = {.button = 0xFF,
                       .leds = 0,
                       .handler = error_handler},
    [MODE_STARTUP_CHECK] = {.button = 0xFF,
                            .leds = 0,
                            .handler = show_startup_errors},
    [MODE_SILENCE] = {.button = 0xFF,
                      .leds = 0,
                      .handler = silence_channels}
};

static uint8_t mode_stack[3];
static uint8_t mode_stack_pointer = 0;

static inline void mode_stack_push(uint8_t m)
{
    if (mode_stack_pointer >= 3)
        while (1); // should not happen

    mode_stack[mode_stack_pointer] = m;
    mode_stack_pointer++;
}

static inline uint8_t mode_stack_pop(void)
{
    if (mode_stack_pointer == 0)
        while (1); // should not happen

    mode_stack_pointer--;
    return mode_stack[mode_stack_pointer];
}

void ui_push_mode(uint8_t m)
{
    mode_stack_push(mode);
    if (!modes[m].leds) {
        for (uint8_t i = 0; i < 24; i++) {
            leds_off(i);
        }
    }
    button_leds = modes[m].leds;
    mode = m;
}

void ui_pop_mode()
{
    uint8_t m = mode_stack_pop();
    button_leds = modes[m].leds;
    mode = m;
}

void ui_handler(void)
/*
  Top level user interface handler. Checks whether one of the
  mode buttons have been pressed, and transfers control to
  the corresponding function.
*/
{
    if (mode <= MODE_SETTINGS) {
        button_led_on(BTN_PAGE1 + mode);
        for (enum mode m = MODE_PAGE1; m <= MODE_SETTINGS; m++) {
            if (button_pressed(modes[m].button)) {
                button_led_off(modes[mode].button);
                mode = m;
                button_leds = modes[m].leds;
            }
        }
    }

    modes[mode].handler();

    // Save current button states
    prev_input[0] = input[0];
    prev_input[1] = input[1];
    prev_input[2] = input[2];
}


#define LED_STATE_OFF 0
#define LED_STATE_ON 1
#define LED_STATE_BLINK 2

void ui_leds_handler(void)
/*
   Handles the updating of all button LEDs.
   If a button's value is 0xFF, it is on, and
   if it's 0, it's off. Otherwise the value is the
   current value of a counter which is used to blink
   the LEDs.
*/
{
    static uint8_t counter;

    if (counter++ == BLINK_CNT)
        counter = 0;

    if (button_leds == 0)
        return;

    for (uint8_t i = 0; i < 24; i++) {
        if (button_led_get(i) == LED_STATE_OFF)
            leds_off(i);

        else if (button_led_get(i) == LED_STATE_ON)
            leds_on(i);

        else  {
            if (counter == BLINK_CNT) {
                leds_toggle(i);
            }
        }
    }
}


/*
   Functions used by the user interface handlers
*/

#define WAIT_CNT 60
#define SPEED_CNT 10

uint8_t ui_updown(int8_t* value, int8_t min, int8_t max)
/* Handles up/down buttons when selecting values */
{
    static uint8_t wait_count = 0;
    static uint8_t speed_count = 0;

    if ((button_on(BTN_UP) && *value < max) || (button_on(BTN_DOWN) && *value > min)) {
        // If the button was just pressed, increase/decrease one step and start the wait counter
        // (to avoid the value rapidly changing if the button is held for too long)
        if (button_pressed(BTN_UP)) {
            (*value)++;
            wait_count = 0;
            return 1;
        }

        else if (button_pressed(BTN_DOWN)) {
            (*value)--;
            wait_count = 0;
            return 1;
        }

        // If the button was not just pressed, increase the wait counter and see if
        // we can start to increase the value continuously
        else {
            if (wait_count == WAIT_CNT) {
                if (speed_count++ == SPEED_CNT) {
                    speed_count = 0;
                    *value = (button_on(BTN_UP)) ? *value + 1 : *value - 1;
                    return 1;
                }
            }
            else
                wait_count++;
        }
    }
    return 0;
}

struct getvalue_config getvalue = {.state = INACTIVE};

static void getvalue_handler(void)
/* Handles getting a parameter value. A new getvalue-session is initiated by
   using ui_getvalue_session. This structure holds the current parameter to
   be changed, which buttons to blink, and the state.
*/
{
    static int8_t value;

    // If the state just changed to GETVALUE, set the value to the parameter's
    // current value
    if (getvalue.state == INACTIVE) {
        if (getvalue.parameter.type == INVRANGE)
            value = getvalue.parameter.max - *getvalue.parameter.target;
        else
            value = *getvalue.parameter.target;

        button_led_blink(getvalue.button1);

        if (getvalue.button2 != 0xFF)
            button_led_blink(getvalue.button2);

        getvalue.midi_note = 0xFF;
        getvalue.state = ACTIVE;
    }

    // Handle up and down buttons
    ui_updown(&value, getvalue.parameter.min, getvalue.parameter.max);

    if (getvalue.parameter.type == NOTE && getvalue.midi_note != 0xFF) {
        button_led_off(getvalue.button1);
        if (getvalue.button2 != 0xFF)
            button_led_off(getvalue.button2);
        getvalue.state = INACTIVE;
        mode = getvalue.previous_mode;
        *getvalue.parameter.target = getvalue.midi_note;
    }

    // When SET is pressed, store the new value in the parameter and disable LED blinking.
    // If type is VALTYPE_INVRANGE, the value is inverted.
    if (button_pressed(BTN_SAVE)) {
        if (getvalue.parameter.type == INVRANGE)
            *getvalue.parameter.target = getvalue.parameter.max - value;
        else
            *getvalue.parameter.target = value;

        button_led_off(getvalue.button1);

        if (getvalue.button2 != 0xFF)
            button_led_off(getvalue.button2);

        getvalue.state = INACTIVE;

        mode = getvalue.previous_mode;
    }

    if (getvalue.parameter.type == NOTE) {
        leds_7seg_note_set(3, 4, value);
    }
    else if (getvalue.parameter.type == SCALE) {
        leds_7seg_two_digit_set(3, 4, 1 << value);
    }
    else if (getvalue.parameter.type == KBD_HALF) {
        if (value == 1) {
            leds[3] = 0b01111100; // U
            leds[4] = 0b11001110; // p
        }
        else {
            leds[3] = 0b00011100; // L
            leds[4] = 0b00111010; // o
        }
    }
    else if (getvalue.parameter.min < 0) {
        if (value < 0) {
            leds_7seg_minus(3);
            leds_7seg_set(4, -value);
        }
        else {
            leds_7seg_clear(3);
            leds_7seg_set(4, value);
        }
    }
    else
        leds_7seg_two_digit_set(3, 4, value);
}

#define BATT_FLASH_DURATION 64

uint8_t ui_startup_errors;

static void silence_channels(void)
{
    static uint8_t count = 0;
    static uint8_t enable = 0;

    if (count == 0) {
        for (uint8_t i = 0; i < 4; i++) {
            enable |= assigner_enabled[i] << i;
            assigner_enabled[i] = 1;
            play_note(i, 24);
        }
    }
    if (count == 1) {
        for (uint8_t i = 0; i < 4; i++) {
            stop_note(i);
            assigner_enabled[i] = (enable >> i) & 1;
        }
        mode = MODE_STARTUP_CHECK;
    }
    count++;
}

static void show_startup_errors(void)
{
    // Done when error mask is empty
    if (!ui_startup_errors) {
        mode = MODE_PAGE1;
        return;
    }

    static uint8_t flash_count;
    static uint8_t duration_count;
    static uint8_t disp_on = 1;
    static uint8_t current_error = 0;

    if (!(ui_startup_errors & (1 << current_error))) {
        current_error++;
        return;
    }

    leds[3] = 0;
    leds[4] = 0;
    if (disp_on) {
        switch (current_error) {
        case UI_STARTUP_ERROR_BATTERY_LOW:
            leds[3] = 0b00111110; // b
            leds[4] = 0b00011100; // L
            break;
        case UI_STARTUP_ERROR_CORRUPT_RAM:
            leds[3] = 0b00011010; // c
            leds[4] = 0b00001010; // r
            break;
        }
    }

    if (++duration_count == BATT_FLASH_DURATION) {
        duration_count = 0;
        disp_on ^= 1;
        if (disp_on && ++flash_count == 6) {
            flash_count = 0;
            ui_startup_errors &= ~(1 << current_error);
        }
    }
}

void transfer(void)
{
    leds[3] = 0b00001010; // s
    leds[4] = 0b00001010; // A

    for (uint8_t i = 0; i < 16; i++) {
        if (midi_transfer_progress > i)
            leds_on(i);
    }
}

static uint8_t error_mask;

void error_set(uint8_t error_bit)
{
    error_mask |= error_bit;
    ui_push_mode(MODE_ERROR);
}

static void error_handler(void)
{
    for (uint8_t i = 0; i < 7; i++) {
        if (error_mask & (1 << i))
            leds_on(i);
        else
            leds_off(i);
    }
    leds_7seg_set(3, 0b10011110);
    leds_7seg_set(4, 0b00001010);

    if (button_pressed(BTN_SAVE))
        ui_pop_mode();
}
