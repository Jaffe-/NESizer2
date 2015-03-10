/*
  NESIZER
  General user interface routines
  
  (c) Johan Fjeldtvedt

  Handles the user interface when in programming mode.
*/


#include "ui.h"
#include "ui_programmer.h"
#include <avr/pgmspace.h>
#include "patch.h"
#include "leds.h"
#include "apu.h"

#define BTN_LFO1 5 // 21
#define BTN_LFO2 6 // 22
#define BTN_LFO3 7 // 23

#define BTN_DUTY 8
#define BTN_LOOP 8
#define BTN_WAVE 9
#define BTN_GLIDE 9
#define BTN_DETUNE 10
#define BTN_ENVMOD 11
#define BTN_OCTAVE 0
#define BTN_SAMPLEFREQ 11
#define BTN_A 12
#define BTN_D 13
#define BTN_S 14
#define BTN_R 15

#define PATCH_MIN 0
#define PATCH_MAX 99

typedef enum { STATE_TOPLEVEL, STATE_SAVE } State;

State state = STATE_TOPLEVEL;

#define SIZE(ARR) (sizeof(ARR) / sizeof(ARR[0]))

const ParameterID sq1_parameters[] PROGMEM = {
  SQ1_LFO1,
  SQ1_LFO2,
  SQ1_LFO3,
  SQ1_DUTY,
  SQ1_DETUNE,
  SQ1_GLIDE,
  SQ1_ENVMOD,
  ENV1_ATTACK,
  ENV1_DECAY,
  ENV1_SUSTAIN,
  ENV1_RELEASE,
  SQ1_PITCHBEND,
  SQ1_OCTAVE
};

const ParameterID sq2_parameters[] PROGMEM = {
  SQ2_LFO1,
  SQ2_LFO2,
  SQ2_LFO3,
  SQ2_DUTY,
  SQ2_DETUNE,
  SQ2_GLIDE,
  SQ2_ENVMOD,
  ENV2_ATTACK,
  ENV2_DECAY,
  ENV2_SUSTAIN,
  ENV2_RELEASE,
  SQ2_PITCHBEND,
  SQ2_OCTAVE
};

const ParameterID tri_parameters[] PROGMEM = {
  TRI_LFO1,
  TRI_LFO2,
  TRI_LFO3,    
  0xFF,
  TRI_DETUNE,
  TRI_GLIDE,
  TRI_ENVMOD,
  0xFF,
  0xFF,
  0xFF,
  0xFF,
  TRI_PITCHBEND,
  TRI_OCTAVE
};

const ParameterID noise_parameters[] PROGMEM = {
  NOISE_LFO1,
  NOISE_LFO2,
  NOISE_LFO3,
  NOISE_LOOP,
  0xFF,
  0xFF,
  NOISE_ENVMOD,
  ENV3_ATTACK,
  ENV3_DECAY,
  ENV3_SUSTAIN,
  ENV3_RELEASE
};

const ParameterID dmc_parameters[] PROGMEM = {
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
  0xFF,
};

const ParameterID lfo1_parameters[] PROGMEM = {
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
  0xFF,
    
};

const ParameterID lfo2_parameters[] PROGMEM = {
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
  0xFF,
    
};

const ParameterID lfo3_parameters[] PROGMEM = {
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
  0xFF,
    
};

typedef struct {
  const ParameterID* parameter_list;
  ParameterID depress_parameter;
} MainButton;

MainButton main_buttons[] = {
  {sq1_parameters, SQ1_ENABLED},
  {sq2_parameters, SQ2_ENABLED},
  {tri_parameters, TRI_ENABLED},
  {noise_parameters, NOISE_ENABLED},
  {dmc_parameters, DMC_ENABLED},
  {lfo1_parameters, LFO1_PERIOD},
  {lfo2_parameters, LFO2_PERIOD},
  {lfo3_parameters, LFO3_PERIOD}
};

static inline void toplevel_handler();

void programmer()
/* Handles the front panel functions when at the top level 
   
   The whole thing is basically a large loop going through the array
   of possible button presses and combinations. 
*/
{
  static uint8_t patchno = 0;

  // Display the selected patch number
  leds_7seg_two_digit_set(3, 4, patchno);

  uint8_t last_patchno = patchno;
  // Handle UP and DOWN presses
  ui_updown(&patchno, PATCH_MIN, PATCH_MAX);

  if (state == STATE_SAVE) {
    patch_save(patchno);
    state = STATE_TOPLEVEL;
  }
  else if (patchno != last_patchno) 
    patch_load(patchno);
    
  if (button_pressed(BTN_SAVE)) {
    ui_getvalue_session.button1 = BTN_SAVE;
    ui_getvalue_session.button2 = 0xFF;
    ui_getvalue_session.parameter.target = &patchno;
    ui_getvalue_session.parameter.type = VALTYPE_RANGE;
    ui_getvalue_session.parameter.min = PATCH_MIN;
    ui_getvalue_session.parameter.max = PATCH_MAX;
    mode |= MODE_GETVALUE;
    state = STATE_SAVE;
    return;
  }

  button_leds[BTN_SQ1] = sq1.enabled * 0xFF;
  button_leds[BTN_SQ2] = sq2.enabled * 0xFF;
  button_leds[BTN_TRI] = tri.enabled * 0xFF;
  button_leds[BTN_NOISE] = noise.enabled * 0xFF;
  button_leds[BTN_DMC] = dmc.enabled * 0xFF;

  toplevel_handler(); 
}

static inline ParameterID get_parameter_id(uint8_t main_button, uint8_t parameter_button)
{
  if (parameter_button != 0xFF)
    return pgm_read_byte(&main_buttons[main_button].parameter_list[parameter_button - 5]);
  return 0xFF;
}

static inline void toplevel_handler()
{
  uint8_t parameter_button = 0xFF;

  // Search through the list of parameter buttons pressed, and find the first
  for (uint8_t i = 5; i < 16; i++) {
    if (button_pressed(i)) {
      if (button_on(BTN_SHIFT))
	parameter_button = i + 8;
      else
	parameter_button = i;
      break;
    }
  }

  for (uint8_t i = 0; i < 8; i++) {
    // Handle the case where the main button is currently being pressed
    if (button_on(i)) {
      ParameterID id = get_parameter_id(i, parameter_button);

      // If a parameter button is being pressed and is a valid button for
      // the current main button, set up a getvalue session.
      if (id != 0xFF) {
	ui_getvalue_session.button1 = i;
	ui_getvalue_session.button2 = parameter_button;
	ui_getvalue_session.parameter = parameter_get(id);
	mode |= MODE_GETVALUE;
	return;
      }
    }

    // In the case where the main button has been depressed
    else if (button_depressed(i)) {
      Parameter parameter = parameter_get(main_buttons[i].depress_parameter);

      if (parameter.type == VALTYPE_BOOL) 
	*parameter.target ^= 1;
      else {
	ui_getvalue_session.button1 = i;
	ui_getvalue_session.button2 = 0xFF;
	ui_getvalue_session.parameter = parameter;
	mode |= MODE_GETVALUE;
	return;
      }
    }
  }
}
