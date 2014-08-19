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

static uint8_t mode = MODE_PROGRAMMER;

uint8_t prev_input[24] = {0};
uint8_t button_leds[24] = {0};

void ui_handler()
{
    switch (mode) {
    case MODE_PROGRAMMER: programmer(); break;
    case MODE_SEQUENCER: sequencer(); break;
    case MODE_SETTINGS: break;
    }

    // Some manual checks:
    if (button_pressed(BTN_PRG)) 
	mode = MODE_PROGRAMMER;
    else if (button_pressed(BTN_SEQ))
	mode = MODE_SEQUENCER;
    else if (button_pressed(BTN_SYS))
	mode = MODE_SETTINGS;

    button_leds[BTN_PRG] = (mode == MODE_PROGRAMMER) * 0xFF;
    button_leds[BTN_SEQ] = (mode == MODE_SEQUENCER) * 0xFF;
    button_leds[BTN_SYS] = (mode == MODE_SETTINGS) * 0xFF;

//    bperiods[2] = (input[BTN_TST]) ? 400 : 0;
    bperiods[0] = 400;
    env1.gate = input[BTN_TST];

    for (uint8_t i = 0; i < 24; i++) 
	prev_input[i] = input[i];
      
}

void ui_leds_handler()
{
    programmer_leds();
}
