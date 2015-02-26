/*
  NESIZER
  General user interface routines
  
  (c) Johan Fjeldtvedt

  Contains the UI handler which checks button presses and transfers control to 
  the corresponding mode handlers, and the LED refresh handler.
 */

#include "ui.h"
#include "ui_sequencer.h"
#include "ui_programmer.h"
#include "ui_settings.h"
#include "leds.h"
#include "input.h"

#define BLINK_CNT 30

uint8_t mode = MODE_PROGRAM;

uint8_t prev_input[3] = {0};
uint8_t button_leds[24] = {0};

void ui_handler()
/*
  Top level user interface handler. Checks whether one of the
  mode buttons have been pressed, and transfers control to 
  the corresponding function.
 */
{
//    static const 
    static uint8_t last_mode;

    last_mode = mode;
    
    if (!(mode & MODE_GETVALUE)) {
	if (button_pressed(BTN_PROGRAM)) 
	    mode = MODE_PROGRAM;
	else if (button_pressed(BTN_PATTERN))
	    mode = MODE_PATTERN;
	else if (button_pressed(BTN_TRACK))
	    mode = MODE_TRACK;
	else if (button_pressed(BTN_SETTINGS))
	    mode = MODE_SETTINGS;
    }

    if (mode & MODE_GETVALUE)
	ui_getvalue_handler();
    else {
	switch (mode) {
	case MODE_PROGRAM: programmer(); break;
	case MODE_PATTERN: sequencer(); break;
	case MODE_TRACK: break;  // not implemented yet!
	case MODE_SETTINGS: settings(); break;
	}
    }
    
    // When mode switches, clear all button LEDs
    if ((mode & 0x7F) != (last_mode & 0x7F)) {
	for (uint8_t i = 0; i < 24; i++) 
	    button_leds[i] = 0;	
	leds_7seg_clear(3);
	leds_7seg_clear(4);	
    }
    
    // Indicate which mode is chosen
    button_leds[BTN_PROGRAM] = (mode == MODE_PROGRAM) * 0xFF;
    button_leds[BTN_PATTERN] = (mode == MODE_PATTERN) * 0xFF;
    button_leds[BTN_TRACK] = (mode == MODE_TRACK) * 0xFF;
    button_leds[BTN_SETTINGS] = (mode == MODE_SETTINGS) * 0xFF;
    
    // Save current button states
    prev_input[0] = input[0];
    prev_input[1] = input[1];
    prev_input[2] = input[2];
}

void ui_leds_handler()
/* Handles the updating of all button LEDs.
   If a button's value is 0xFF, it is on, and
   if it's 0, it's off. Otherwise the value is the
   current value of a counter which is used to blink
   the LEDs. 
*/
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


/* 
   Functions used by the user interface handlers
*/

#define WAIT_CNT 60
#define SPEED_CNT 10

void ui_updown(uint8_t* value, uint8_t min, uint8_t max)
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
	}
	
	else if (button_pressed(BTN_DOWN)) {
	    (*value)--;
	    wait_count = 0;
	}
	
	// If the button was not just pressed, increase the wait counter and see if 
	// we can start to increase the value continuously
	else {
	    if (wait_count == WAIT_CNT) {
		if (speed_count++ == SPEED_CNT) {
		    speed_count = 0;
		    *value = (button_on(BTN_UP)) ? *value + 1 : *value - 1;
		}
	    }
	    else 
		wait_count++;
	}
    }
}

GetvalueSession ui_getvalue_session = {.state = SESSION_INACTIVE};

void ui_getvalue_handler()
/* Handles getting a parameter value. A new getvalue-session is initiated by
   using ui_getvalue_session. This structure holds the current parameter to
   be changed, which buttons to blink, and the state. 
 */
{
    static uint8_t value;
    
    // If the state just changed to GETVALUE, set the value to the parameter's value
    // and last pot value to the current. 
    if (ui_getvalue_session.state == SESSION_INACTIVE) {
	value = (ui_getvalue_session.parameter.type == VALTYPE_INVRANGE) ?
	    ui_getvalue_session.parameter.max - *ui_getvalue_session.parameter.target 
	    : *ui_getvalue_session.parameter.target;
	
	button_leds[ui_getvalue_session.button1] = 1;
	leds_set(ui_getvalue_session.button1, 0);

	if (ui_getvalue_session.button2 != 0xFF) {
	    button_leds[ui_getvalue_session.button2] = 1;
	    leds_set(ui_getvalue_session.button2, 0);
	}

	ui_getvalue_session.state = SESSION_ACTIVE;
    }

    // Handle up and down buttons
    ui_updown(&value, ui_getvalue_session.parameter.min, ui_getvalue_session.parameter.max);

    // When SET is pressed, store the new value in the parameter and disable LED blinking.
    // If type is VALTYPE_INVRANGE, the value is inverted. 

    if (button_pressed(BTN_SET)) {
	if (ui_getvalue_session.parameter.type == VALTYPE_INVRANGE)
	    *ui_getvalue_session.parameter.target = ui_getvalue_session.parameter.max - value;
	else
	    *ui_getvalue_session.parameter.target = value;
	
	button_leds[ui_getvalue_session.button1] = 0;

	if (ui_getvalue_session.button2 != 0xFF) 
	    button_leds[ui_getvalue_session.button2] = 0;

	ui_getvalue_session.state = SESSION_INACTIVE;
	
	mode &= 0x7F;
    }	    

    if (ui_getvalue_session.parameter.type != VALTYPE_POLRANGE) {
	// Display the value on the 7 segment displays
	leds_7seg_two_digit_set(3, 4,  value);
    }
    else {
	uint8_t midpoint = (ui_getvalue_session.parameter.max - ui_getvalue_session.parameter.min) / 2;
	if (value < midpoint) {
	    leds_7seg_set(3, LEDS_7SEG_MINUS);
	    leds_7seg_set(4, midpoint - value);
	}
	else {
	    leds_7seg_clear(3);
	    leds_7seg_set(4, value - midpoint);
	}
    }

}
