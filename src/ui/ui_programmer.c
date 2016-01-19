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



  Patch programmer user interface

  Handles the user interface when in programming mode.
*/


#include "ui.h"
#include "ui_programmer.h"
#include <avr/pgmspace.h>
#include "patch/patch.h"
#include "apu/apu.h"

#define BTN_LFO1 5 // 21
#define BTN_LFO2 6 // 22
#define BTN_LFO3 7 // 23

#define BTN_DUTY 8
#define BTN_LOOP 8

#define BTN_WAVE 9
#define BTN_GLIDE 9

#define BTN_DETUNE 10

#define BTN_ENVMOD 11
#define BTN_SAMPLEFREQ 11

#define BTN_A 12

#define BTN_D 13

#define BTN_S 14

#define BTN_R 15

#define BTN_OCTAVE 0


#define PATCH_MIN 0
#define PATCH_MAX 99

uint8_t programmer_leds[6];

enum state { STATE_TOPLEVEL, STATE_SAVE };

enum state state = STATE_TOPLEVEL;

#define SIZE(ARR) (sizeof(ARR) / sizeof(ARR[0]))

const enum parameter_id sq1_parameters[] PROGMEM = {
  SQ1_LFO1,
  SQ1_LFO2,
  SQ1_LFO3,

  SQ1_DUTY,
  SQ1_DETUNE,
  SQ1_GLIDE,
  SQ1_PITCHBEND,
  ENV1_ATTACK,
  ENV1_DECAY,
  ENV1_SUSTAIN,
  ENV1_RELEASE
};

const enum parameter_id sq2_parameters[] PROGMEM = {
  SQ2_LFO1,
  SQ2_LFO2,
  SQ2_LFO3,

  SQ2_DUTY,
  SQ2_DETUNE,
  SQ2_GLIDE,
  SQ2_PITCHBEND,
  ENV2_ATTACK,
  ENV2_DECAY,
  ENV2_SUSTAIN,
  ENV2_RELEASE
};

const enum parameter_id tri_parameters[] PROGMEM = {
  TRI_LFO1,
  TRI_LFO2,
  TRI_LFO3,

  0xFF,
  TRI_DETUNE,
  TRI_GLIDE,
  TRI_PITCHBEND,
  0xFF,
  0xFF,
  0xFF,
  0xFF
};

const enum parameter_id noise_parameters[] PROGMEM = {
  NOISE_LFO1,
  NOISE_LFO2,
  NOISE_LFO3,

  NOISE_LOOP,
  0xFF,
  0xFF,
  NOISE_PITCHBEND,
  ENV3_ATTACK,
  ENV3_DECAY,
  ENV3_SUSTAIN,
  ENV3_RELEASE
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

  0xFF,
  LFO1_WAVEFORM,
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

  0xFF,
  LFO2_WAVEFORM,
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

  0xFF,
  LFO3_WAVEFORM,
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

struct main_button main_buttons[] = {
  {sq1_parameters, SQ1_ENABLED},
  {sq2_parameters, SQ2_ENABLED},
  {tri_parameters, TRI_ENABLED},
  {noise_parameters, NOISE_ENABLED},
  {dmc_parameters, DMC_ENABLED},
  {lfo1_parameters, LFO1_PERIOD},
  {lfo2_parameters, LFO2_PERIOD},
  {lfo3_parameters, LFO3_PERIOD}
};

static inline void toplevel_handler(void);

void programmer(void)
/* Handles the front panel functions when at the top level 
   
   The whole thing is basically a large loop going through the array
   of possible button presses and combinations. 
*/
{
  static int8_t patchno = 0;

  // Display the selected patch number
  leds_7seg_two_digit_set(3, 4, patchno);

  int8_t last_patchno = patchno;
  // Handle UP and DOWN presses
  ui_updown(&patchno, PATCH_MIN, PATCH_MAX);

  if (state == STATE_SAVE) {
    patch_save(patchno);
    state = STATE_TOPLEVEL;
  }
  else if (patchno != last_patchno) 
    patch_load(patchno);
    
  if (button_pressed(BTN_SAVE)) {
    getvalue.button1 = BTN_SAVE;
    getvalue.button2 = 0xFF;
    getvalue.parameter.target = &patchno;
    getvalue.parameter.type = RANGE;
    getvalue.parameter.min = PATCH_MIN;
    getvalue.parameter.max = PATCH_MAX;
    getvalue.previous_mode = mode;
    state = STATE_SAVE;
    return;
  }

  sq1.enabled ? button_led_on(BTN_SQ1) : button_led_off(BTN_SQ1);
  sq2.enabled ? button_led_on(BTN_SQ2) : button_led_off(BTN_SQ2);
  tri.enabled ? button_led_on(BTN_TRI) : button_led_off(BTN_TRI);
  noise.enabled ? button_led_on(BTN_NOISE) : button_led_off(BTN_NOISE);
  dmc.enabled ? button_led_on(BTN_DMC) : button_led_off(BTN_DMC);

  toplevel_handler(); 
}

static inline enum parameter_id get_parameter_id(uint8_t main_button, uint8_t parameter_button)
{
  if (parameter_button != 0xFF) {
    uint8_t parameter_num = parameter_button - 5;
    return pgm_read_byte(&main_buttons[main_button].parameter_list[parameter_num]);
  }
  else
    return 0xFF;
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

  for (uint8_t i = 0; i < 8; i++) {
    // Handle the case where the main button is currently being pressed
    if (button_on(i)) {
      enum parameter_id id = get_parameter_id(i, parameter_button);

      // If a parameter button is being pressed and is a valid button for
      // the current main button, set up a getvalue session.
      if (id != 0xFF) {
	getvalue.button1 = i;
	getvalue.button2 = parameter_button;
	getvalue.parameter = parameter_get(id);
	mode |= MODE_GETVALUE;
	return;
      }
    }

    // In the case where the main button has been depressed
    else if (button_depressed(i)) {
      struct parameter parameter = parameter_get(main_buttons[i].depress_parameter);

      if (parameter.type == BOOL) 
	*parameter.target ^= 1;
      else {
	getvalue.button1 = i;
	getvalue.button2 = 0xFF;
	getvalue.parameter = parameter;
	mode |= MODE_GETVALUE;
	return;
      }
    }
  }
}
