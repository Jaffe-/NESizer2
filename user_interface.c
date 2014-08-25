#include <avr/io.h>
#include <avr/pgmspace.h>
#include "user_interface.h"
#include "input.h"
#include "leds.h"
#include "apu.h"
#include "envelope.h"
#include "lfo.h"
#include "modulation.h"
#include "timing.h"
#include "sequencer.h"
#include "programmer.h"

#define WAIT_CNT 60
#define SPEED_CNT 10
#define BLINK_CNT 30

static uint8_t mode = MODE_PROGRAMMER;

uint8_t prev_input[3] = {0};
uint8_t button_leds[24] = {0};

void updown(uint8_t* value, uint8_t min, uint8_t max)
/* Handles up/down buttons when selecting values */
{
    static uint8_t updown_wait_count = 0;
    static uint8_t updown_speed_count = 0;

    if ((button_on(BTN_UP) && *value < max) || (button_on(BTN_DOWN) && *value > min)) {
	// If the button was just pressed, increase/decrease one step and start the wait counter
	// (to avoid the value rapidly changing if the button is held for too long)
	if (button_pressed(BTN_UP)) {
	    (*value)++;
	    updown_wait_count = 0;
	}
	
	else if (button_pressed(BTN_DOWN)) {
	    (*value)--;
	    updown_wait_count = 0;
	}
	
	// If the button was not just pressed, increase the wait counter and see if 
	// we can start to increase the value continuously
	else {
	    if (updown_wait_count == WAIT_CNT) {
		if (updown_speed_count++ == SPEED_CNT) {
		    updown_speed_count = 0;
		    *value = (button_on(BTN_UP)) ? *value + 1 : *value - 1;
		}
	    }
	    else 
		updown_wait_count++;
	}
    }
}

void ui_handler()
{
    static uint8_t last_mode;

    last_mode = mode;

    // Some manual checks:
    if (button_pressed(BTN_PRG)) 
	mode = MODE_PROGRAMMER;
    else if (button_pressed(BTN_SEQ))
	mode = MODE_SEQUENCER;
    else if (button_pressed(BTN_SYS))
	mode = MODE_SETTINGS;

    switch (mode) {
    case MODE_PROGRAMMER: programmer(); break;
    case MODE_SEQUENCER: sequencer(); break;
    case MODE_SETTINGS: break;
    }

    // When mode switches, clear all button LEDs
    if (mode != last_mode) {
	for (uint8_t i = 0; i < 24; i++) 
	    button_leds[i] = 0;	
	leds_7seg_clear(3);
	leds_7seg_clear(4);
	
    }

    // Indicate which mode is chosen
    button_leds[BTN_PRG] = (mode == MODE_PROGRAMMER) * 0xFF;
    button_leds[BTN_SEQ] = (mode == MODE_SEQUENCER) * 0xFF;
    button_leds[BTN_SYS] = (mode == MODE_SETTINGS) * 0xFF;

    prev_input[0] = input[0];
    prev_input[1] = input[1];
    prev_input[2] = input[2];

}

void ui_leds_handler()
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
