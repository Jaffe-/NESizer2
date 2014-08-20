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

static uint8_t counter;

#define cnt 30

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
#define Cs5 Cs4 >> 1 // Find correct!

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

#define pos_to_btn(POS) (((POS) < 8) ? ((POS) + 16) : (POS))
#define btn_to_pos(BTN) (((BTN) < 16) ? (BTN) : ((BTN) - 16))

inline void wait_note()
{
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

inline void program()
{
    if (button_pressed(BTN_PLAY)) {
	state = STATE_PLAY;
	current_pos = 0;
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
		button_leds[i] = 1; // blink the corresponding LED
		break;
	    }
	}
    }
}

inline void play()
{
    
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
	
	if (++current_pos == 16) current_pos = 0;
		 
    }
}

void sequencer()
{
    switch (state) {
    case STATE_WAIT_NOTE:
	wait_note(); break;
    case STATE_PROGRAM:
	program(); break;
    case STATE_PLAY:
	play(); break;
    }

    // Display the chosen pattern
    leds_7seg_set(3, current_pattern + 1);

    // Put values in upper 16 button LEDs
    for (uint8_t i = 8; i < 24; i++) {
	switch (state) {
	case STATE_PROGRAM:
	    button_leds[i] = (patterns[current_pattern][btn_to_pos(i)] != 0) * 0xFF;
	    break;
	case STATE_WAIT_NOTE:
	    // Skip the current position led, since it is set to blink
	    if (i != pos_to_btn(current_pos))
		button_leds[i] = (patterns[current_pattern][btn_to_pos(i)] != 0) * 0xFF;
	    break;
	case STATE_PLAY:
	    button_leds[i] = (i == pos_to_btn(current_pos)) * 0xFF;
	    break;
	}
    }
}
