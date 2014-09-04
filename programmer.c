#include <avr/io.h>
#include <avr/pgmspace.h>
#include "programmer.h"
#include "user_interface.h"
#include "ui_library.h"
#include "input.h"
#include "leds.h"
#include "apu.h"
#include "envelope.h"
#include "lfo.h"
#include "modulation.h"
#include "patch.h"

#define BTN_LFO1 21
#define BTN_LFO2 22
#define BTN_LFO3 23

#define BTN_DUTY 8
#define BTN_LOOP 8
#define BTN_WAVE 9
#define BTN_PORTAMENTO 9
#define BTN_DETUNE 10
#define BTN_OCTAVE 11
#define BTN_SAMPLEFREQ 11
#define BTN_A 12
#define BTN_D 13
#define BTN_S 14
#define BTN_R 15

static uint8_t state = STATE_TOPLEVEL;

static uint8_t octave_shift[3] = {2, 2, 2};

Parameter current_parameter;

#define SIZE(ARR) (sizeof(ARR) / sizeof(ARR[0]))

const Parameter sq1_parameters[] PROGMEM = {
    {BTN_DUTY, &sq1.duty, VALTYPE_RANGE, 0, 3},
    {BTN_A, &env1.attack, VALTYPE_RANGE, 0, 99},
    {BTN_D, &env1.decay, VALTYPE_RANGE, 0, 99},
    {BTN_S, &env1.sustain, VALTYPE_RANGE, 0, 15},
    {BTN_R, &env1.release, VALTYPE_RANGE, 0, 99},
    {BTN_LFO1, &lfo_mod_matrix[0][0], VALTYPE_RANGE, 0, 60},
    {BTN_LFO2, &lfo_mod_matrix[0][1], VALTYPE_RANGE, 0, 60},
    {BTN_LFO3, &lfo_mod_matrix[0][2], VALTYPE_RANGE, 0, 60},
    {BTN_DETUNE, &detune[0], VALTYPE_POLRANGE, 0, 18},
    {BTN_OCTAVE, &octave_shift[0], VALTYPE_POLRANGE, 0, 4}
};

const Parameter sq2_parameters[] PROGMEM = {
    {BTN_DUTY, &sq2.duty, VALTYPE_RANGE, 0, 3},
    {BTN_A, &env2.attack, VALTYPE_RANGE, 0, 99},
    {BTN_D, &env2.decay, VALTYPE_RANGE, 0, 99},
    {BTN_S, &env2.sustain, VALTYPE_RANGE, 0, 15},
    {BTN_R, &env2.release, VALTYPE_RANGE, 0, 99},
    {BTN_LFO1, &lfo_mod_matrix[1][0], VALTYPE_RANGE, 0, 60},
    {BTN_LFO2, &lfo_mod_matrix[1][1], VALTYPE_RANGE, 0, 60},
    {BTN_LFO3, &lfo_mod_matrix[1][2], VALTYPE_RANGE, 0, 60},
    {BTN_DETUNE, &detune[1], VALTYPE_POLRANGE, 0, 18},
    {BTN_OCTAVE, &octave_shift[0], VALTYPE_POLRANGE, 0, 4}
};

const Parameter tri_parameters[] PROGMEM = {
    {BTN_LFO1, &lfo_mod_matrix[2][0], VALTYPE_RANGE, 0, 60},
    {BTN_LFO2, &lfo_mod_matrix[2][1], VALTYPE_RANGE, 0, 60},
    {BTN_LFO3, &lfo_mod_matrix[2][2], VALTYPE_RANGE, 0, 60},
    {BTN_DETUNE, &detune[2], VALTYPE_POLRANGE, 0, 18},
    {BTN_OCTAVE, &octave_shift[0], VALTYPE_POLRANGE, 0, 4}
};

const Parameter noise_parameters[] PROGMEM = {
    {BTN_LFO1, &lfo_mod_matrix[3][0], VALTYPE_RANGE, 0, 60},
    {BTN_LFO2, &lfo_mod_matrix[3][1], VALTYPE_RANGE, 0, 60},
    {BTN_LFO3, &lfo_mod_matrix[3][2], VALTYPE_RANGE, 0, 60},
    {BTN_A, &env3.attack, VALTYPE_RANGE, 0, 99},
    {BTN_D, &env3.decay, VALTYPE_RANGE, 0, 99},
    {BTN_S, &env3.sustain, VALTYPE_RANGE, 0, 15},
    {BTN_R, &env3.release, VALTYPE_RANGE, 0, 99},
    {BTN_LOOP, &noise.loop, VALTYPE_BOOL, 0, 0}
};

const Parameter dmc_parameters[] PROGMEM = {
    {BTN_DMC, &dmc.sample_number, VALTYPE_RANGE, 1, 3},
    {BTN_LOOP, &dmc.sample_loop, VALTYPE_BOOL, 0, 0}
};

const Parameter lfo1_parameters[] PROGMEM = {
    {BTN_WAVE, &lfo1.waveform, VALTYPE_RANGE, 1, 3},
};

const Parameter lfo2_parameters[] PROGMEM = {
    {BTN_WAVE, &lfo2.waveform, VALTYPE_RANGE, 1, 3},
};

const Parameter lfo3_parameters[] PROGMEM = {
    {BTN_WAVE, &lfo3.waveform, VALTYPE_RANGE, 1, 3},
};

MainButton main_buttons[] = {
    {BTN_SQ1, BTNSTATE_NOTPRESSED, sq1_parameters, SIZE(sq1_parameters), &sq1.enabled, VALTYPE_BOOL},
    {BTN_SQ2, BTNSTATE_NOTPRESSED, sq2_parameters, SIZE(sq2_parameters), &sq2.enabled, VALTYPE_BOOL},
    {BTN_TRI, BTNSTATE_NOTPRESSED, tri_parameters, SIZE(tri_parameters), &tri.enabled, VALTYPE_BOOL},
    {BTN_NOISE, BTNSTATE_NOTPRESSED, noise_parameters, SIZE(noise_parameters), &noise.enabled, VALTYPE_BOOL},
    {BTN_DMC, BTNSTATE_NOTPRESSED, tri_parameters, SIZE(tri_parameters), &dmc.enabled, VALTYPE_BOOL},
    {BTN_LFO1, BTNSTATE_NOTPRESSED, lfo1_parameters, SIZE(lfo1_parameters), &lfo1.period, VALTYPE_RANGE},
    {BTN_LFO2, BTNSTATE_NOTPRESSED, lfo2_parameters, SIZE(lfo2_parameters), &lfo2.period, VALTYPE_RANGE},
    {BTN_LFO3, BTNSTATE_NOTPRESSED, lfo3_parameters, SIZE(lfo3_parameters), &lfo3.period, VALTYPE_RANGE},
};

static inline void toplevel();
//static inline void getvalue();

void programmer()
{
    static uint8_t state_changed;
    uint8_t last_state = state;

    switch (state) {
    case STATE_TOPLEVEL: 
	toplevel(); break;
    case STATE_GETVALUE: 
	getvalue_handler(&state, state_changed); break;
    }

    // TODO: make this less hard coded
    if (state == STATE_TOPLEVEL) {
	button_leds[BTN_SQ1] = sq1.enabled * 0xFF;
	button_leds[BTN_SQ2] = sq2.enabled * 0xFF;
	button_leds[BTN_TRI] = tri.enabled * 0xFF;
	button_leds[BTN_NOISE] = noise.enabled * 0xFF;
	button_leds[BTN_DMC] = dmc.enabled * 0xFF;
    }

    state_changed = (last_state != state);
}

static inline void toplevel()
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
    updown(&patchno, PATCH_MIN, PATCH_MAX);
    if (patchno != last_patchno) 
	patch_load(patchno);

    if (button_pressed(BTN_SAVE)) {
	current_parameter.button = BTN_SAVE;
	current_parameter.target = &patchno;
	current_parameter.type = VALTYPE_RANGE;
	current_parameter.min = PATCH_MIN;
	current_parameter.max = PATCH_MAX;
	main_button = BTN_SAVE;
	state = STATE_GETVALUE;
	return;
    }

    toplevel_handler(main_buttons, SIZE(main_buttons), &state); 
}

uint8_t programmer_mode_switchable()
/* Returns wether or not the mode can be switched from SETTINGS */
{
    return (state != STATE_GETVALUE);
}
