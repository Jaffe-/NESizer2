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

#define BLINK_CNT 30
#define WAIT_CNT 60
#define SPEED_CNT 10

#define VALTYPE_BOOL 0
#define VALTYPE_RANGE 1
#define VALTYPE_INVRANGE 2

#define NO_BTN 0xFF

#define STATE_TOPLEVEL 0
#define STATE_GETVALUE 1

static uint8_t state = STATE_TOPLEVEL;
static uint8_t state_changed;

typedef struct {
    uint8_t button1;   
    uint8_t button2;
    uint8_t* target;
    uint8_t type;
    uint8_t min;
    uint8_t max;
} ButtonCombination;

ButtonCombination cur_comb;

// Array of button combination definitions.

const ButtonCombination combinations[] PROGMEM = {
    // SQ1 combinations:
    {BTN_SQ1, 0xFF, &sq1.enabled, VALTYPE_BOOL, 0, 0},
    {BTN_SQ1, BTN_DUTY, &sq1.duty, VALTYPE_RANGE, 0, 3},
    {BTN_SQ1, BTN_A, &env1.attack, VALTYPE_RANGE, 0, 99},
    {BTN_SQ1, BTN_D, &env1.decay, VALTYPE_RANGE, 0, 99},
    {BTN_SQ1, BTN_S, &env1.sustain, VALTYPE_RANGE, 0, 15},
    {BTN_SQ1, BTN_R, &env1.release, VALTYPE_RANGE, 0, 99},
    {BTN_SQ1, BTN_LFO1, &lfo_mod_matrix[0][0], VALTYPE_RANGE, 0, 60},
    {BTN_SQ1, BTN_LFO2, &lfo_mod_matrix[1][0], VALTYPE_RANGE, 0, 60},
    {BTN_SQ1, BTN_LFO3, &lfo_mod_matrix[2][0], VALTYPE_RANGE, 0, 60},

    // SQ2 combinations:
    {BTN_SQ2, 0xFF, &sq2.enabled, VALTYPE_BOOL, 0, 0},
    {BTN_SQ2, BTN_DUTY, &sq2.duty, VALTYPE_RANGE, 0, 3},
    {BTN_SQ2, BTN_A, &env2.attack, VALTYPE_RANGE, 0, 99},
    {BTN_SQ2, BTN_D, &env2.decay, VALTYPE_RANGE, 0, 99},
    {BTN_SQ2, BTN_S, &env2.sustain, VALTYPE_RANGE, 0, 15},
    {BTN_SQ2, BTN_R, &env2.release, VALTYPE_RANGE, 0, 99},
    {BTN_SQ2, BTN_LFO1, &lfo_mod_matrix[0][1], VALTYPE_RANGE, 0, 60},
    {BTN_SQ2, BTN_LFO2, &lfo_mod_matrix[1][1], VALTYPE_RANGE, 0, 60},
    {BTN_SQ2, BTN_LFO3, &lfo_mod_matrix[2][1], VALTYPE_RANGE, 0, 60},

    // TRI combinations:
    {BTN_TRI, 0xFF, &tri.enabled, VALTYPE_BOOL, 0, 0},
    {BTN_TRI, BTN_LFO1, &lfo_mod_matrix[0][2], VALTYPE_RANGE, 0, 60},
    {BTN_TRI, BTN_LFO2, &lfo_mod_matrix[1][2], VALTYPE_RANGE, 0, 60},
    {BTN_TRI, BTN_LFO3, &lfo_mod_matrix[2][2], VALTYPE_RANGE, 0, 60},

    // NOISE combinations:
    {BTN_NOISE, 0xFF, &noise.enabled, VALTYPE_BOOL, 0, 0},
    {BTN_NOISE, BTN_A, &env3.attack, VALTYPE_RANGE, 0, 99},
    {BTN_NOISE, BTN_D, &env3.decay, VALTYPE_RANGE, 0, 99},
    {BTN_NOISE, BTN_S, &env3.sustain, VALTYPE_RANGE, 0, 15},
    {BTN_NOISE, BTN_R, &env3.release, VALTYPE_RANGE, 0, 99},

    // DMC
    {BTN_DMC, 0xFF, &dmc.enabled, VALTYPE_BOOL, 0, 0},

    // LFOs
    {BTN_LFO1, BTN_WAVE, &lfo1.waveform, VALTYPE_RANGE, 1, 3},
    {BTN_LFO1, 0xFF, &lfo1.period, VALTYPE_INVRANGE, 0, 99},

    {BTN_LFO2, BTN_WAVE, &lfo2.waveform, VALTYPE_RANGE, 1, 3},
    {BTN_LFO2, 0xFF, &lfo2.period, VALTYPE_INVRANGE, 0, 99},

    {BTN_LFO3, BTN_WAVE, &lfo3.waveform, VALTYPE_RANGE, 1, 3},
    {BTN_LFO3, 0xFF, &lfo3.period, VALTYPE_INVRANGE, 0, 99},

    {BTN_LOOP, 0xFF, &noise.loop, VALTYPE_BOOL, 0, 0}
};

inline void toplevel();
inline void getvalue();

void programmer()
{
    uint8_t last_state = state;

    switch (state) {
    case STATE_TOPLEVEL: 
	toplevel(); break;
    case STATE_GETVALUE: 
	getvalue(); break;
    }

    state_changed = (last_state != state);
}

inline void toplevel()
/* Handles the front panel functions when at the top level 

   The whole thing is basically a large loop going through the array
   of possible button presses and combinations. 
*/
{

    for (uint8_t i = 0; i < 35; i++) {
	// Get data out of flash
	ButtonCombination comb = {
	    .button1 = pgm_read_byte(&combinations[i].button1),
	    .button2 = pgm_read_byte(&combinations[i].button2),
	    .target = (uint8_t*)pgm_read_word(&combinations[i].target),
	    .type = pgm_read_byte(&combinations[i].type),
	    .min = pgm_read_byte(&combinations[i].min),
	    .max = pgm_read_byte(&combinations[i].max)
	};
	
	if ((input[comb.button1] && comb.button2 != NO_BTN && input[comb.button2]) 
	    || (comb.button2 == NO_BTN && button_depressed(comb.button1))) {
	    if (comb.type != VALTYPE_BOOL) { 
		state = STATE_GETVALUE;
		cur_comb = comb;
		
		button_leds[comb.button1] = BLINK_CNT;
		leds_set(comb.button1, 0);
		if (comb.button2 != NO_BTN) {
		    button_leds[comb.button2] = BLINK_CNT;
		    leds_set(comb.button2, 0);
		}
	    }
	    else 
		*comb.target ^= 1;
	    
	    break;
	}
	if (comb.type == VALTYPE_BOOL && button_depressed(comb.button1)) {
	    *comb.target ^= 1;
	    break;
	    }
    }
    
    // Unless in the GETVALUE state, LEDs should indicate the state of their corresponding parameter
    if (state != STATE_GETVALUE) {
	button_leds[BTN_SQ1] = sq1.enabled * 0xFF;
	button_leds[BTN_SQ2] = sq2.enabled * 0xFF;
	button_leds[BTN_TRI] = tri.enabled * 0xFF;
	button_leds[BTN_NOISE] = noise.enabled * 0xFF;
	button_leds[BTN_DMC] = dmc.enabled * 0xFF;
	button_leds[BTN_LOOP] = noise.loop * 0xFF;
    }    
}

inline void getvalue()
/* Handles getting a parameter value */
{
    static uint8_t value;
    static uint8_t last_pot_value;
    static uint8_t updown_wait_count = 0;
    static uint8_t updown_speed_count = 0;
    
    // Get analog pot value and scale it to fit the parameter's range
    uint8_t pot_value = cur_comb.min + input_analog_value * (cur_comb.max + 1 - cur_comb.min) / 256;

    // If the state just changed to GETVALUE, set the value to the parameter's value
    // and last pot value to the current. 
    if (state_changed) {
	value = *cur_comb.target;
	last_pot_value = pot_value;
    }

    // A change in the pot should immediately set the value to its value
    if (pot_value != last_pot_value) {
	value = pot_value;
	last_pot_value = pot_value;
    }

    // Handle up and down buttons
    if ((input[BTN_UP] && value < cur_comb.max) || (input[BTN_DOWN] && value > cur_comb.min)) {
	// If the button was just pressed, increase/decrease one step and start the wait counter
	// (to avoid the value rapidly changing if the button is held for too long)
	if (button_pressed(BTN_UP)) {
	    value++;
	    updown_wait_count = 0;
	}

	else if (button_pressed(BTN_DOWN)) {
	    value--;
	    updown_wait_count = 0;
	}

	// If the button was not just pressed, increase the wait counter and see if 
	// we can start to increase the value continuously
	else {
	    if (updown_wait_count == WAIT_CNT) {
		if (updown_speed_count++ == SPEED_CNT) {
		    updown_speed_count = 0;
		    value = (input[BTN_UP]) ? value + 1 : value - 1;
		}
	    }
	    else 
		updown_wait_count++;
	}
    }
    
    // When SET is pressed, store the new value in the parameter and disable LED blinking.
    // If type is VALTYPE_INVRANGE, the value is inverted. 
    else if (button_pressed(BTN_SET)) {
	if (cur_comb.type == VALTYPE_RANGE)
	    *cur_comb.target = value;
	else
	    *cur_comb.target = cur_comb.max - value;
	
	button_leds[cur_comb.button1] = 0;
	button_leds[cur_comb.button2] = 0;
	
	state = STATE_TOPLEVEL;
    }	    

    // Display the value on the 7 segment displays
    leds_7seg_two_digit_set(3, 4, value);

}

void programmer_leds()
{
    for (uint8_t i = 0; i < 24; i++) {
	if (button_leds[i] == 0) 
	    leds[i / 8] &= ~(1 << (i % 8));
	else if (button_leds[i] == 0xFF)
	    leds[i / 8] |= 1 << (i % 8);
	else  {
	    if (--button_leds[i] == 0) {
		button_leds[i] = BLINK_CNT;
		leds[i / 8] ^= 1 << (i % 8);
	    }
	}
    }

}
