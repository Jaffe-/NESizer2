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
#include "assigner/assigner.h"

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
#define BTN_ARP_SYNC 12

#define BTN_D 13
#define BTN_MONO 13

#define BTN_S 14
#define BTN_POLY1 14

#define BTN_R 15
#define BTN_POLY2 15

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
  0xFF,
  ENV1_ATTACK,
  ENV1_DECAY,
  ENV1_SUSTAIN,
  ENV1_RELEASE
};

const enum parameter_id p2_sq1_parameters[] PROGMEM = {
  SQ1_COARSE,
  SQ1_PITCHBEND,
  SQ1_VOLMOD
};

const enum parameter_id sq2_parameters[] PROGMEM = {
  SQ2_LFO1,
  SQ2_LFO2,
  SQ2_LFO3,

  SQ2_DUTY,
  SQ2_DETUNE,
  SQ2_GLIDE,
  0xFF,
  ENV2_ATTACK,
  ENV2_DECAY,
  ENV2_SUSTAIN,
  ENV2_RELEASE
};

const enum parameter_id p2_sq2_parameters[] PROGMEM = {
  SQ2_COARSE,
  SQ2_PITCHBEND,
  SQ2_VOLMOD
};

const enum parameter_id tri_parameters[] PROGMEM = {
  TRI_LFO1,
  TRI_LFO2,
  TRI_LFO3,

  0xFF,
  TRI_DETUNE,
  TRI_GLIDE,
  0xFF,
  0xFF,
  0xFF,
  0xFF,
  0xFF
};

const enum parameter_id p2_tri_parameters[] PROGMEM = {
  TRI_COARSE,
  TRI_PITCHBEND,
  0xFF
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
  0xFF,
  NOISE_PITCHBEND,
  NOISE_VOLMOD
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
  {0, ARP_RANGE},
  {0, ARP_MODE},
  {0, ARP_CHANNEL},
  {0, ARP_RATE},
  {0, ARP_SYNC}
};

static struct main_button* main_buttons;
static uint8_t main_buttons_length;
static uint8_t param_length;

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
    mode = MODE_GETVALUE;
    state = STATE_SAVE;
    return;
  }

  sq1.enabled ? button_led_on(BTN_SQ1) : button_led_off(BTN_SQ1);
  sq2.enabled ? button_led_on(BTN_SQ2) : button_led_off(BTN_SQ2);
  tri.enabled ? button_led_on(BTN_TRI) : button_led_off(BTN_TRI);
  noise.enabled ? button_led_on(BTN_NOISE) : button_led_off(BTN_NOISE);
  dmc.enabled ? button_led_on(BTN_DMC) : button_led_off(BTN_DMC);

  // In page 2 we also want to indicate the assigner mode
  mode == MODE_PAGE2 && assigner_mode == 1 ?
    button_led_on(BTN_MONO) : button_led_off(BTN_MONO);
  mode == MODE_PAGE2 && assigner_mode == 2 ?
    button_led_on(BTN_POLY1) : button_led_off(BTN_POLY1);
  mode == MODE_PAGE2 && assigner_mode == 3 ?
    button_led_on(BTN_POLY2) : button_led_off(BTN_POLY2);
  mode == MODE_PAGE2 && assigner_arp_sync ?
    button_led_on(BTN_ARP_SYNC) : button_led_off(BTN_ARP_SYNC);

  if (mode == MODE_PAGE1) {
    main_buttons = p1_main_buttons;
    main_buttons_length = sizeof(p1_main_buttons)/sizeof(p1_main_buttons[0]);
    param_length = 11;
  }
  else {
    main_buttons = p2_main_buttons;
    main_buttons_length = sizeof(p2_main_buttons)/sizeof(p2_main_buttons[0]);
    param_length = 3;
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

  // In page 2 we also need to check the three assigner mode buttons.
  if (mode == MODE_PAGE2) {
    for (int i = 13; i <= 15; i++) {
      if (button_pressed(i)) {
	assigner_mode = i - 12;
      }
    }
  }
}
