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

#define BLINK_CNT 30

static uint8_t mode = MODE_PROGRAMMER;

uint8_t prev_input[3] = {0};
uint8_t button_leds[24] = {0};

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
