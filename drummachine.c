#include <avr/io.h>
#include "drummachine.h"
#include "input.h"
#include "leds.h"

#include <avr/pgmspace.h>
#include "apu.h"
#include "modulation.h"
#include "envelope.h"

#include "snare.c"
#include "kick.c"

#define STATE_PROGRAM 0
#define STATE_PLAY 1
#define STATE_WAIT_NOTE 2

#define BUTTON_NEXT 1
#define BUTTON_PREV 0
#define BUTTON_PLAY 7

uint8_t BD_pat[16] = {0};
uint8_t SD_pat[16] = {0};
uint8_t HH_pat[16] = {0};
uint8_t TRI_pat[16] = {0};
uint8_t SQ1_pat[16] = {0};
uint8_t SQ2_pat[16] = {0};

uint8_t prev_input[24] = {0};

uint8_t state = STATE_PROGRAM;

uint8_t current_pattern = 0;

uint8_t* patterns[6] = {BD_pat, SD_pat, HH_pat, TRI_pat, SQ1_pat, SQ2_pat};

uint8_t current_pos = 0;

#define cnt 30

uint8_t counter;

#define C4 197
#define D4 176
#define E4 156
#define F4 148
#define G4 131
#define A4 118
#define B4 104
#define C5 98

inline uint8_t switch_to_period(uint8_t num)
{
    uint8_t p = 0;
    switch (num) {
    case 0: p = C4; break;
    case 1: p = D4; break;
    case 2: p = E4; break;
    case 3: p = F4; break;
    case 4: p = G4; break;
    case 5: p = A4; break;
    case 6: p = B4; break;
    case 7: p = C5; break;
    }
    return p;
}

#define button_pressed(BTN) (input[BTN] == 1 && prev_input[BTN] == 0)

void drum_task()
{
    if (button_pressed(BUTTON_PLAY)) {
	if (state == STATE_WAIT_NOTE) {
	    patterns[current_pattern][current_pos] = 0;
	    state = STATE_PROGRAM;
	}
	else if (state == STATE_PROGRAM) {
	    state = STATE_PLAY;
	    current_pos = 0;
	    counter = cnt;
	}
	else {
	    state = STATE_PROGRAM;
	    env3.gate = 0;
	    env2.gate = 0;
	    env1.gate = 0;
	    bperiods[2] = 0;
	}
    }
	
    else if (state == STATE_WAIT_NOTE) {
	for (uint8_t i = 0; i < 8; i++) {
	    if (button_pressed(i)) {
		patterns[current_pattern][current_pos] = switch_to_period(i);
		state = STATE_PROGRAM;
	    }
	}
    }

    else if (state == STATE_PROGRAM) {

	if (button_pressed(BUTTON_NEXT) && current_pattern < 5) {
	    current_pattern++;
	}
	
	else if (button_pressed(BUTTON_PREV) && current_pattern > 0)
	    current_pattern--;
	   
	for (uint8_t i = 8; i < 24; i++) {
	    if (button_pressed(i)) {
		if (current_pattern < 3) {
		    if (i < 16)
			patterns[current_pattern][i] ^= 1;
		    else
			patterns[current_pattern][i - 16] ^= 1; 
		    
		}
		else {
		    state = STATE_WAIT_NOTE;
		    current_pos = (i < 16) ? i : i - 16;
		    counter = 4;
		    break;
		}
	    }
	}
    }

    else if (state == STATE_PLAY) {
	if (counter == cnt/2) {
	    env3.gate = 0;
	    env2.gate = 0;
	    env1.gate = 0;
	    bperiods[2] = 0;
	}
	if (counter-- == 0) {
	    counter = cnt;
	    
	    if (BD_pat[current_pos]) {
		dmc.sample = kick_c_raw;
		dmc.sample_length = kick_c_raw_len;
		dmc.sample_enabled = 1;
		dmc.current = 0;
	    }

	    if (SD_pat[current_pos]) {
		dmc.sample = snare_c_raw;
		dmc.sample_length = snare_c_raw_len;
		dmc.sample_enabled = 1;
		dmc.current = 0;
	    }

	    if (HH_pat[current_pos]) {
		env3.gate = 1;
	    }
	
	    bperiods[2] = TRI_pat[current_pos] << 2;
    
	    if (SQ1_pat[current_pos]) {
		env1.gate = 1;
		bperiods[0] = SQ1_pat[current_pos] << 1;
	    }

	    if (SQ2_pat[current_pos]) {
		env2.gate = 1;
		bperiods[1] = SQ2_pat[current_pos] << 1;
	    }

	    if (++current_pos == 16) current_pos = 0;

	}	 
    }

    for (uint8_t i = 0; i < 24; i++) {
	prev_input[i] = input[i];
    }

}

void drum_update_leds()
{
    leds_7seg_set(3, current_pattern + 1);
    if (state == STATE_PROGRAM) 
	leds_7seg_dot_on(3);
    else
	leds_7seg_dot_off(3);

    if (state == STATE_WAIT_NOTE) {
	if (--counter == 0) {
	    counter = 60;
	    if (current_pos < 8) 
		leds[1] = 1 << current_pos;
	    else
		leds[0] = 1 << (current_pos - 8);
	}
	if (counter == 30) {
	    leds[1] = 0;
	    leds[0] = 0;
	}
	    
    }

    if (state == STATE_PROGRAM) {
	leds[0] = 0;
	leds[1] = 0;
	for (uint8_t i = 0; i < 16; i++) {
	    if (i < 8) 
		leds[1] |= ((patterns[current_pattern][i] != 0) << i);
	    else
		leds[0] |= ((patterns[current_pattern][i] != 0) << (i - 8));
	}
    }

    if (state == STATE_PLAY) {
	leds[0] = 0;
	leds[1] = 0;
	if (current_pos < 8) 
	    leds[1] = 1 << current_pos;
	else
	    leds[0] = 1 << (current_pos - 8);
    }

}

