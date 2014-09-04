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
#include "settings.h"

#define BLINK_CNT 30

uint8_t mode = MODE_PROGRAM;

uint8_t prev_input[3] = {0};
uint8_t button_leds[24] = {0};


void ui_handler()
{
    static uint8_t last_mode;

    last_mode = mode;

    // Some manual checks:
    if (!((last_mode == MODE_PROGRAM && !programmer_mode_switchable()) ||
	  (last_mode == MODE_SETTINGS && !settings_mode_switchable()))) {
	if (button_pressed(BTN_PROGRAM)) 
	    mode = MODE_PROGRAM;
	else if (button_pressed(BTN_PATTERN))
	    mode = MODE_PATTERN;
	else if (button_pressed(BTN_TRACK))
	    mode = MODE_TRACK;
	else if (button_pressed(BTN_SETTINGS))
	    mode = MODE_SETTINGS;
    }

    switch (mode) {
    case MODE_PROGRAM: programmer(); break;
    case MODE_PATTERN: sequencer(); break;
    case MODE_TRACK: break;
    case MODE_SETTINGS: settings(); break;
    }

    // When mode switches, clear all button LEDs
    if (mode != last_mode) {
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
