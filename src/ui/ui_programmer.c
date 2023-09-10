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



  Patch programmer user interface

  Handles the user interface when in programming mode.
*/


#include "ui.h"
#include "ui_programmer.h"
#include <avr/pgmspace.h>
#include "patch/patch.h"
#include "apu/apu.h"
#include "assigner/assigner.h"
#include "settings/settings.h"

// Page 2
#define BTN_SPLIT 9
#define BTN_SET_SPLIT 10

#define BTN_LOWER_POLY 12
#define BTN_LOWER_MONO 13
#define BTN_UPPER_POLY 14
#define BTN_UPPER_MONO 15

#define PATCH_MIN 0
#define PATCH_MAX 99

uint8_t programmer_leds[6];

enum state { STATE_TOPLEVEL, STATE_SAVE };

static enum state state = STATE_TOPLEVEL;

static int8_t patchno = 0;

#define SIZE(ARR) (sizeof(ARR) / sizeof(ARR[0]))

const enum parameter_id sq1_parameters[] PROGMEM = {
    SQ1_LFO1,
    SQ1_LFO2,
    SQ1_LFO3,

    SQ1_DUTY,
    SQ1_DETUNE,
    SQ1_COARSE,
    SQ1_GLIDE,
    ENV1_ATTACK,
    ENV1_DECAY,
    ENV1_SUSTAIN,
    ENV1_RELEASE
};

const enum parameter_id p2_sq1_parameters[] PROGMEM = {
    SQ1_PITCHBEND,
    SQ1_ENVMOD,
    SQ1_VOLMOD,
    SQ1_HALF
};

const enum parameter_id sq2_parameters[] PROGMEM = {
    SQ2_LFO1,
    SQ2_LFO2,
    SQ2_LFO3,

    SQ2_DUTY,
    SQ2_DETUNE,
    SQ2_COARSE,
    SQ2_GLIDE,
    ENV2_ATTACK,
    ENV2_DECAY,
    ENV2_SUSTAIN,
    ENV2_RELEASE
};

const enum parameter_id p2_sq2_parameters[] PROGMEM = {
    SQ2_PITCHBEND,
    SQ2_ENVMOD,
    SQ2_VOLMOD,
    SQ2_HALF
};

const enum parameter_id tri_parameters[] PROGMEM = {
    TRI_LFO1,
    TRI_LFO2,
    TRI_LFO3,

    0xFF,
    TRI_DETUNE,
    TRI_COARSE,
    TRI_GLIDE,
    0xFF,
    0xFF,
    0xFF,
    0xFF
};

const enum parameter_id p2_tri_parameters[] PROGMEM = {
    TRI_PITCHBEND,
    TRI_ENVMOD,
    0xFF,
    TRI_HALF
};

const enum parameter_id noise_parameters[] PROGMEM = {
    NOISE_LFO1,
    NOISE_LFO2,
    NOISE_LFO3,

    NOISE_LOOP,
    0xFF,
    0xFF,
    0xFF,
    ENV3_ATTACK,
    ENV3_DECAY,
    ENV3_SUSTAIN,
    ENV3_RELEASE
};

const enum parameter_id p2_noise_parameters[] PROGMEM = {
    NOISE_PITCHBEND,
    NOISE_ENVMOD,
    NOISE_VOLMOD,
    NOISE_HALF
};

const enum parameter_id dmc_parameters[] PROGMEM = {
    0xFF,
    0xFF,
    0xFF,

    DMC_SAMPLE_LOOP,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF
};

const enum parameter_id lfo1_parameters[] PROGMEM = {
    0xFF,
    0xFF,
    0xFF,

    LFO1_WAVEFORM,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF
};

const enum parameter_id lfo2_parameters[] PROGMEM = {
    0xFF,
    0xFF,
    0xFF,

    LFO2_WAVEFORM,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF
};

const enum parameter_id lfo3_parameters[] PROGMEM = {
    0xFF,
    0xFF,
    0xFF,

    LFO3_WAVEFORM,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF
};

struct main_button {
    const enum parameter_id* parameter_list;
    enum parameter_id depress_parameter;
};

struct main_button p1_main_buttons[] = {
    {sq1_parameters, SQ1_ENABLED},
    {sq2_parameters, SQ2_ENABLED},
    {tri_parameters, TRI_ENABLED},
    {noise_parameters, NOISE_ENABLED},
    {dmc_parameters, DMC_ENABLED},
    {lfo1_parameters, LFO1_PERIOD},
    {lfo2_parameters, LFO2_PERIOD},
    {lfo3_parameters, LFO3_PERIOD}
};

struct main_button p2_main_buttons[] = {
    {p2_sq1_parameters, SQ1_ENABLED},
    {p2_sq2_parameters, SQ2_ENABLED},
    {p2_tri_parameters, TRI_ENABLED},
    {p2_noise_parameters, NOISE_ENABLED},
    {0, DMC_ENABLED},
    {0, 0xFF},
    {0, 0xFF},
    {0, 0xFF},
    {0, 0xFF},
    {0, 0xFF},
    {0, 0xFF},
    {0, 0xFF},
    {0, 0xFF},
    {0, 0xFF},
    {0, 0xFF},
    {0, 0xFF}
};

static struct main_button* main_buttons;
static uint8_t main_buttons_length;
static uint8_t param_length;

static inline void toplevel_handler(void);

void ui_programmer_setup(void)
{
    for (uint8_t i = 0; i < 4; i++) {
        stop_note(i);
    }
    patchno = settings_read(PROGRAMMER_SELECTED_PATCH);
    patch_load(patchno);
}

void programmer(void)
/* Handles the front panel functions when at the top level

   The whole thing is basically a large loop going through the array
   of possible button presses and combinations.
*/
{

    // Display the selected patch number
    leds_7seg_two_digit_set(3, 4, patchno);

    // Handle UP and DOWN presses
    if (ui_updown(&patchno, PATCH_MIN, PATCH_MAX)) {
        patch_load(patchno);
        settings_write(PROGRAMMER_SELECTED_PATCH, patchno);
    }

    if (state == STATE_SAVE) {
        patch_save(patchno);
        state = STATE_TOPLEVEL;
    }

    if (button_pressed(BTN_SAVE)) {
        getvalue.button1 = BTN_SAVE;
        getvalue.button2 = 0xFF;
        getvalue.parameter.target = &patchno;
        getvalue.parameter.type = RANGE;
        getvalue.parameter.min = PATCH_MIN;
        getvalue.parameter.max = PATCH_MAX;
        getvalue.previous_mode = mode;
        mode = MODE_GETVALUE;
        state = STATE_SAVE;
        return;
    }

    for (uint8_t chn = 0; chn < 5; chn++)
        assigner_enabled[chn] ? button_led_on(chn) : button_led_off(chn);

    // In page 2 we also want to indicate the assigner mode
    mode == MODE_PAGE2 && assigner_lower_mode == MONO ?
        button_led_on(BTN_LOWER_MONO) : button_led_off(BTN_LOWER_MONO);
    mode == MODE_PAGE2 && assigner_lower_mode == POLY ?
        button_led_on(BTN_LOWER_POLY) : button_led_off(BTN_LOWER_POLY);
    mode == MODE_PAGE2 && assigner_upper_mode == MONO ?
        button_led_on(BTN_UPPER_MONO) : button_led_off(BTN_UPPER_MONO);
    mode == MODE_PAGE2 && assigner_upper_mode == POLY ?
        button_led_on(BTN_UPPER_POLY) : button_led_off(BTN_UPPER_POLY);
    mode == MODE_PAGE2 && assigner_split ?
        button_led_on(BTN_SPLIT) : button_led_off(BTN_SPLIT);

    if (mode == MODE_PAGE1) {
        main_buttons = p1_main_buttons;
        main_buttons_length = sizeof(p1_main_buttons)/sizeof(p1_main_buttons[0]);
        param_length = 11;
    }
    else {
        main_buttons = p2_main_buttons;
        main_buttons_length = sizeof(p2_main_buttons)/sizeof(p2_main_buttons[0]);
        param_length = 4;
    }

    toplevel_handler();
}

static inline enum parameter_id get_parameter_id(uint8_t main_button, uint8_t parameter_button)
{
    if (main_buttons[main_button].parameter_list == 0)
        return 0xFF;
    uint8_t parameter_num = parameter_button - 5;
    return pgm_read_byte(&main_buttons[main_button].parameter_list[parameter_num]);
}

static inline void init_getvalue(uint8_t button1, uint8_t button2,
                                 struct parameter* param)
{
    getvalue.button1 = button1;
    getvalue.button2 = button2;
    getvalue.parameter = *param;
    getvalue.previous_mode = mode;
    mode = MODE_GETVALUE;
}

static inline void toplevel_handler(void)
{
    uint8_t parameter_button = 0xFF;

    // Search through the list of parameter buttons pressed, and find the first
    for (uint8_t i = 5; i < 16; i++) {
        if (button_pressed(i)) {
            parameter_button = i;
            break;
        }
    }

    for (uint8_t i = 0; i < main_buttons_length; i++) {

        // Handle the case where the main button is currently being pressed
        if (button_on(i) && main_buttons[i].parameter_list != 0) {
            if (parameter_button < 5 + param_length) {
                enum parameter_id id = get_parameter_id(i, parameter_button);

                if (id != 0xFF) {
                    struct parameter parameter = parameter_get(id);
                    init_getvalue(i, parameter_button, &parameter);

                    break;
                }
            }
        }

        // In the case where the main button has been depressed
        else if (button_depressed(i) && main_buttons[i].depress_parameter != 0xFF) {
            struct parameter parameter = parameter_get(main_buttons[i].depress_parameter);

            if (parameter.type == BOOL)
                *parameter.target ^= 1;
            else {
                init_getvalue(i, 0xFF, &parameter);

                break;
            }
        }
    }

    if (mode == MODE_PAGE2) {
        if (button_pressed(BTN_LOWER_POLY)) {
            assigner_lower_mode = POLY;
            settings_write(ASSIGNER_LOWER_MODE, POLY);
        }
        if (button_pressed(BTN_LOWER_MONO)) {
            assigner_lower_mode = MONO;
            settings_write(ASSIGNER_LOWER_MODE, MONO);
        }
        if (button_pressed(BTN_UPPER_POLY)) {
            assigner_upper_mode = POLY;
            settings_write(ASSIGNER_UPPER_MODE, POLY);
        }
        if (button_pressed(BTN_UPPER_MONO)) {
            assigner_upper_mode = MONO;
            settings_write(ASSIGNER_UPPER_MODE, MONO);
        }
        if (button_pressed(BTN_SPLIT)) {
            assigner_split = !assigner_split;
            settings_write(ASSIGNER_SPLIT, assigner_split);
        }
        if (button_pressed(BTN_SET_SPLIT)) {
            struct parameter parameter = parameter_get(SPLIT_POINT);
            init_getvalue(BTN_SET_SPLIT, 0xFF, &parameter);
        }
    }
}
