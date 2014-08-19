#include <avr/io.h>
#include <avr/pgmspace.h>
#include "sequencer.h"
#include "user_interface.h"
#include "input.h"
#include "leds.h"
#include "apu.h"
#include "modulation.h"
#include "envelope.h"
#include "snare.c"
#include "kick.c"

#define STATE_PROGRAM 0
#define STATE_PLAY 1
#define STATE_WAIT_NOTE 2

#define BTN_NEXT 1
#define BTN_PREV 0
#define BTN_PLAY 7

uint8_t BD_pat[16] = {0};
uint8_t SD_pat[16] = {0};
uint8_t HH_pat[16] = {0};
uint8_t TRI_pat[16] = {0};
uint8_t SQ1_pat[16] = {0};
uint8_t SQ2_pat[16] = {0};

static uint8_t state = STATE_PROGRAM;

uint8_t current_pattern = 0;

uint8_t* patterns[6] = {BD_pat, SD_pat, HH_pat, TRI_pat, SQ1_pat, SQ2_pat};

uint8_t current_pos = 0;

#define cnt 30

static uint8_t counter;

// Note periods
#define C4 197
#define Cs4 186
#define D4 176
#define Ds4 166
#define E4 156
#define F4 148
#define Fs4 139
#define G4 131
#define Gs4 124
#define A4 118
#define As4 110
#define B4 104
#define C5 98
#define Cs5 95 // Find correct!

// "Keyboard" keys when in note entering state
#define BTN_C 8
#define BTN_D 9
#define BTN_E 10
#define BTN_F 11
#define BTN_G 12
#define BTN_A 13
#define BTN_B 14
#define BTN_C2 15
#define BTN_Cs 16
#define BTN_Ds 17
#define BTN_Fs 19
#define BTN_Gs 20
#define BTN_As 21
#define BTN_Cs2 23

inline uint8_t switch_to_period(uint8_t num)
{
    uint8_t p = 0;
    switch (num) {
    case BTN_C: p = C4; break;
    case BTN_Cs: p = Cs4; break;
    case BTN_D: p = D4; break;
    case BTN_Ds: p = Ds4; break;
    case BTN_E: p = E4; break;
    case BTN_F: p = F4; break;
    case BTN_Fs: p = Fs4; break;
    case BTN_G: p = G4; break;
    case BTN_Gs: p = Gs4; break;
    case BTN_A: p = A4; break;
    case BTN_As: p = As4; break;
    case BTN_B: p = B4; break;
    case BTN_C2: p = C5; break;
    case BTN_Cs2: p = Cs5; break;
    }
    return p;
}

#define pos_to_led(POS) (((POS) < 8) ? ((POS) + 16) : ((POS) + 8))
#define btn_to_pos(BTN) (((BTN) < 16) ? (BTN) : ((BTN) - 16))

void sequencer()
{
    leds_7seg_set(3, current_pattern + 1);

    if (state == STATE_WAIT_NOTE) {
	if (button_pressed(BTN_PLAY)) {
	    patterns[current_pattern][current_pos] = 0;
	    state = STATE_PROGRAM;
	}

	for (uint8_t i = 8; i < 24; i++) {
	    if (button_pressed(i)) {
		patterns[current_pattern][current_pos] = switch_to_period(i);
		state = STATE_PROGRAM;
	    }
	}
    }

    else if (state == STATE_PROGRAM) {
	for (uint8_t i = 8; i < 24; i++)
	    button_leds[i] = (patterns[current_pattern][btn_to_pos(i)] != 0);

	if (button_pressed(BTN_PLAY)) {
	    state = STATE_PLAY;
	    current_pos = 0;
	    counter = cnt;
	}

	if (button_pressed(BTN_NEXT) && current_pattern < 5) 
	    current_pattern++;
	
	else if (button_pressed(BTN_PREV) && current_pattern > 0)
	    current_pattern--;
	   
	for (uint8_t i = 8; i < 24; i++) {
	    if (button_pressed(i)) {
		if (current_pattern < 3) 
		    patterns[current_pattern][btn_to_pos(i)] ^= 1;

		else {
		    state = STATE_WAIT_NOTE;
		    current_pos = btn_to_pos(i);
		    counter = 4;
		    button_leds[i] = 1;
		    break;
		}
	    }
	}
    }

    else if (state == STATE_PLAY) {
	if (button_pressed(BTN_PLAY)) {
	    state = STATE_PROGRAM;
	    env3.gate = 0;
	    env2.gate = 0;
	    env1.gate = 0;
	    bperiods[2] = 0;

	}

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

	    button_leds[pos_to_led(current_pos)] = 0;
	    if (++current_pos == 16) current_pos = 0;
	    button_leds[pos_to_led(current_pos)] = 0xFF;

	}	 
    }
}

/*
void sequencer_leds()
{
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

*/
