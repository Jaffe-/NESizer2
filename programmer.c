#include <avr/io.h>
#include <avr/pgmspace.h>
#include "programmer.h"
#include "user_interface.h"
#include "input.h"
#include "leds.h"
#include "apu.h"
#include "envelope.h"
#include "lfo.h"
#include "modulation.h"
#include "patch.h"

#define VALTYPE_BOOL 0
#define VALTYPE_RANGE 1
#define VALTYPE_INVRANGE 2

#define NO_BTN_DEPRESS 0x80
#define NO_BTN_PRESS 0x81

#define no_btn(VAL) (((VAL) & 0x80) != 0)

#define STATE_TOPLEVEL 0
#define STATE_GETVALUE 1

static uint8_t state = STATE_TOPLEVEL;
static uint8_t state_changed;
static uint8_t patchno = 0;

typedef struct {
    uint8_t button;
    uint8_t* target;
    uint8_t type;
    uint8_t min;
    uint8_t max;
} Parameter;

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
    {BTN_LFO3, &lfo_mod_matrix[0][2], VALTYPE_RANGE, 0, 60}
};

const Parameter sq2_parameters[] PROGMEM = {
    {BTN_DUTY, &sq2.duty, VALTYPE_RANGE, 0, 3},
    {BTN_A, &env2.attack, VALTYPE_RANGE, 0, 99},
    {BTN_D, &env2.decay, VALTYPE_RANGE, 0, 99},
    {BTN_S, &env2.sustain, VALTYPE_RANGE, 0, 15},
    {BTN_R, &env2.release, VALTYPE_RANGE, 0, 99},
    {BTN_LFO1, &lfo_mod_matrix[1][0], VALTYPE_RANGE, 0, 60},
    {BTN_LFO2, &lfo_mod_matrix[1][1], VALTYPE_RANGE, 0, 60},
    {BTN_LFO3, &lfo_mod_matrix[1][2], VALTYPE_RANGE, 0, 60}
};

const Parameter tri_parameters[] PROGMEM = {
    {BTN_LFO1, &lfo_mod_matrix[2][0], VALTYPE_RANGE, 0, 60},
    {BTN_LFO2, &lfo_mod_matrix[2][1], VALTYPE_RANGE, 0, 60},
    {BTN_LFO3, &lfo_mod_matrix[2][2], VALTYPE_RANGE, 0, 60}
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

typedef struct {
    uint8_t button;
    uint8_t state;
    const Parameter* parameter_list;
    uint8_t parameter_list_length;
    uint8_t* depress_parameter;
    uint8_t depress_parameter_type;
} MainButton;

#define BTNSTATE_NOTPRESSED 0
#define BTNSTATE_PRESSED 1
#define BTNSTATE_PARAM 2

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
static inline void getvalue();

void programmer()
{
    uint8_t last_state = state;

    switch (state) {
    case STATE_TOPLEVEL: 
	toplevel(); break;
    case STATE_GETVALUE: 
	getvalue(); break;
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

uint8_t main_button;

static inline void toplevel()
/* Handles the front panel functions when at the top level 
   
   The whole thing is basically a large loop going through the array
   of possible button presses and combinations. 
*/
{
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

    for (uint8_t i = 0; i < SIZE(main_buttons); i++) {
	
	if (button_pressed(main_buttons[i].button)) 
	    main_buttons[i].state = BTNSTATE_PRESSED;
	
	else if (button_depressed(main_buttons[i].button)) {
	    
	    // If nothing more happened than pressing the button, a depress should change the channel's
	    // state in case of the channel buttons, or LFO speed in case of the LFOs.
	    if (main_buttons[i].state == BTNSTATE_PRESSED) {
		switch (main_buttons[i].depress_parameter_type) {
		    
		case VALTYPE_RANGE:
		    current_parameter.button = main_buttons[i].button;
		    current_parameter.target = main_buttons[i].depress_parameter;
		    current_parameter.type = VALTYPE_INVRANGE;
		    current_parameter.min = 0;
		    current_parameter.max = 99;
		    main_button = main_buttons[i].button;
		    state = STATE_GETVALUE;
		    break;
		    
		case VALTYPE_BOOL:
		    *(main_buttons[i].depress_parameter) ^= 1;
		    
		}
	    }

	    for (uint8_t j = 0; j < main_buttons[i].parameter_list_length; j++) {
		if (pgm_read_byte(&(main_buttons[i].parameter_list[j].type)) == VALTYPE_BOOL) 
		    button_leds[pgm_read_byte(&(main_buttons[i].parameter_list[j].button))] = 0;
	    }

	    main_buttons[i].state = BTNSTATE_NOTPRESSED;
	    
	}

	if (button_on(main_buttons[i].button))
	    for (uint8_t j = 0; j < main_buttons[i].parameter_list_length; j++) {
		Parameter parameter = {.button = pgm_read_byte(&(main_buttons[i].parameter_list[j].button)),
				       .target = (uint8_t*)pgm_read_word(&(main_buttons[i].parameter_list[j].target)),
				       .type = pgm_read_byte(&(main_buttons[i].parameter_list[j].type)),
				       .min = pgm_read_byte(&(main_buttons[i].parameter_list[j].min)),
				       .max = pgm_read_byte(&(main_buttons[i].parameter_list[j].max))};
	    
		if (parameter.type == VALTYPE_BOOL) 
		    button_leds[parameter.button] = (*parameter.target != 0) * 0xFF;
	    
		if (button_pressed(parameter.button)) {
		    switch (parameter.type) {
		    case VALTYPE_RANGE:
		    case VALTYPE_INVRANGE:
			main_button = main_buttons[i].button;
			current_parameter = parameter;
			state = STATE_GETVALUE;
			break;
			
		    case VALTYPE_BOOL:
			*parameter.target ^= 1;
			
		    }

		    main_buttons[i].state = BTNSTATE_PARAM;
		    
		    
		}
	    }
	
    }
}

static inline void getvalue()
/* Handles getting a parameter value */
{
    static uint8_t value;
    static uint8_t last_pot_value;
    
    // Get analog pot value and scale it to fit the parameter's range
    uint8_t pot_value = current_parameter.min + input_analog_value * (current_parameter.max + 1 - current_parameter.min) / 256;

    // If the state just changed to GETVALUE, set the value to the parameter's value
    // and last pot value to the current. 
    if (state_changed) {
	value = (current_parameter.type == VALTYPE_INVRANGE) ? current_parameter.max - *current_parameter.target 
	                                                     : *current_parameter.target;
	last_pot_value = pot_value;

	// Make the LEDs blink to indicate which value is being altered
	button_leds[main_button] = 1;
	button_leds[current_parameter.button] = 1;
	leds_set(main_button, 0);
	leds_set(current_parameter.button, 0);
    }

    // A change in the pot should immediately set the value to its value
    if (pot_value != last_pot_value) {
	value = pot_value;
	last_pot_value = pot_value;
    }

    // Handle up and down buttons
    updown(&value, current_parameter.min, current_parameter.max);

    // Special case for the SAVE button
    if (current_parameter.button == BTN_SAVE && (button_pressed(BTN_SAVE))) {
	patch_save(value);
	patchno = value;

	button_leds[current_parameter.button] = 0;
	
	state = STATE_TOPLEVEL;
    }

    // When SET is pressed, store the new value in the parameter and disable LED blinking.
    // If type is VALTYPE_INVRANGE, the value is inverted. 
    else if (current_parameter.button != BTN_SAVE && button_pressed(BTN_SET)) {
	if (current_parameter.type == VALTYPE_RANGE)
	    *current_parameter.target = value;
	else
	    *current_parameter.target = current_parameter.max - value;
	
	button_leds[main_button] = 0;
	button_leds[current_parameter.button] = 0;
	
	state = STATE_TOPLEVEL;
    }	    

    // Display the value on the 7 segment displays
    leds_7seg_two_digit_set(3, 4,  value);

}

