#include <avr/io.h>
#include <avr/pgmspace.h>
#include "sequencer.h"
#include "user_interface.h"
#include "input.h"
#include "leds.h"
#include "apu.h"
#include "modulation.h"
#include "envelope.h"
#include "patch.h"
#include "memory.h"
#include "sample.h"

#define STATE_PROGRAM 0
#define STATE_PLAY 1
#define STATE_WAIT_NOTE 2
#define STATE_TOPLEVEL 3

#define BTN_PLAY 0

#define cnt 30

// Note periods
#define Gs1 2005
#define A1 1892
#define As1 1786
#define B1 1686

#define C2 1591
#define Cs2 1502
#define D2 1417
#define Ds2 1338
#define E2 1262
#define F2 1192
#define Fs2 1125
#define G2 1062 
#define Gs2 1002
#define A2 946
#define As2 892
#define B2 842

#define C3 795
#define Cs3 750
#define D3 708
#define Ds3 668
#define E3 631
#define F3 595
#define Fs3 562
#define G3 530
#define Gs3 500
#define A3 472
#define As3 446
#define B3 421

#define C4 397
#define Cs4 375
#define D4 354
#define Ds4 334
#define E4 315
#define F4 297
#define Fs4 280
#define G4 265
#define Gs4 250
#define A4 237
#define As4 222
#define B4 210

#define C5 198
#define Cs5 187
#define D5 176
#define Ds5 166
#define E5 157
#define F5 148
#define Fs5 140
#define G5 132
#define Gs5 124
#define A5 117
#define As5 111
#define B5 104

#define C6 99
#define Cs6 93
#define D6 88
#define Ds6 83
#define E6 78
#define F6 74
#define Fs6 69
#define G6 65
#define Gs6 62
#define A6 58
#define As6 55
#define B6 52

#define C7 49

const uint16_t period_table[7][12] PROGMEM = {
    {0, 0, 0, 0, 0, 0, 0, 0, Gs1, A1, As1, B1},
    {C2, Cs2, D2, Ds2, E2, F2, Fs2, G2, Gs2, A2, As2, B2},
    {C3, Cs3, D3, Ds3, E3, F3, Fs3, G3, Gs3, A3, As3, B3},
    {C4, Cs4, D4, Ds4, E4, F4, Fs4, G4, Gs4, A4, As4, B4},
    {C5, Cs5, D5, Ds5, E5, F5, Fs5, G5, Gs5, A5, As5, B5},
    {C6, Cs6, D6, Ds6, E6, F6, Fs6, G6, Gs6, A6, As6, B6},
    {C7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// "Keyboard" keys when in note entering state
#define BTN_NOTE_C 8
#define BTN_NOTE_D 9
#define BTN_NOTE_E 10
#define BTN_NOTE_F 11
#define BTN_NOTE_G 12
#define BTN_NOTE_A 13
#define BTN_NOTE_B 14
#define BTN_NOTE_C2 15
#define BTN_NOTE_Cs 16
#define BTN_NOTE_Ds 17
#define BTN_NOTE_Fs 19
#define BTN_NOTE_Gs 20
#define BTN_NOTE_As 21

#define BTN_CLEAR 23
#define BTN_OCTAVE 22

/* Current pattern 
DMC array stores sample numbers.
NOISE array stores lengths only.
TRI and SQ arrays store in the following format:

XXOOONNNN

where XX is a 2-bit length value, OOO is a 3-bit octave value and NNNN is the note number.
*/

uint16_t DMC_pat[16] = {0};
uint16_t NOISE_pat[16] = {0};
uint16_t TRI_pat[16] = {0};
uint16_t SQ1_pat[16] = {0};
uint16_t SQ2_pat[16] = {0};

static uint8_t state = STATE_TOPLEVEL;

uint8_t current_pattern = 0;
uint8_t current_channel = 0;

uint16_t* patterns[5] = {SQ1_pat, SQ2_pat, TRI_pat, NOISE_pat, DMC_pat};

uint8_t current_pos = 0;

static uint8_t counter;

inline uint8_t switch_to_note(uint8_t num) 
{
    switch (num) {
    case BTN_NOTE_C: return 0;
    case BTN_NOTE_Cs: return 1;
    case BTN_NOTE_D: return 2;
    case BTN_NOTE_Ds: return 3;
    case BTN_NOTE_E: return 4;
    case BTN_NOTE_F: return 5;
    case BTN_NOTE_Fs: return 6;
    case BTN_NOTE_G: return 7;
    case BTN_NOTE_Gs: return 8;
    case BTN_NOTE_A: return 9;
    case BTN_NOTE_As: return 10;
    case BTN_NOTE_B: return 11;
    case BTN_NOTE_C2: return 12;
    }
    // Illegal otherwise
    return 0xFF;
}

inline uint16_t note_to_period(uint8_t octave, uint8_t note)
{
    if (note == 12) {
	octave++;
	note = 0;
    }
    return pgm_read_word(&(period_table[octave][note]));
}

inline uint16_t switch_to_period(uint8_t octave, uint8_t num) 
{
    return note_to_period(octave, switch_to_note(num));
}

inline uint8_t note_length(uint16_t value) 
{
    return (value >> 8) & 0x0F;
}

inline uint8_t note_octave(uint16_t value)
{
    return (value >> 4) & 0x0F;
}

inline uint8_t note_value(uint16_t value)
{
    return value & 0x0F;
}

static uint16_t pack_note_value(uint8_t octave, uint8_t note, uint8_t length)
{
    return note | (octave << 4) | (length << 8);
}

static void insert_pattern_note(uint8_t channel, uint8_t position, uint8_t octave, uint8_t note, uint8_t length)
{
    patterns[channel][position] = pack_note_value(octave, note, length);
}

static void clear_pattern_note(uint8_t channel, uint8_t position)
{
    patterns[channel][position] = 0;
}

static void play_note(uint8_t channel, uint16_t value, uint8_t count);

#define pos_to_btn(POS) (((POS) < 8) ? ((POS) + 16) : (POS))
#define btn_to_pos(BTN) (((BTN) < 16) ? (BTN) : ((BTN) - 16))

uint8_t sq1_counter;
uint8_t sq2_counter;
uint8_t tri_counter;
uint8_t noise_counter;
uint8_t dmc_counter;

static inline void wait_note()
{
    static uint8_t note;
    static uint8_t octave;
    static uint8_t length;

    uint8_t change = 0;

    if (button_pressed(BTN_CLEAR)) {
	clear_pattern_note(current_channel, current_pos);
	state = STATE_PROGRAM;
    }

    else if (!button_pressed(BTN_OCTAVE) && button_on(BTN_OCTAVE)) {
	leds_7seg_set(4, octave + 1);
	if (button_pressed(BTN_UP) || button_pressed(BTN_DOWN)) {
	    if (button_pressed(BTN_UP) && octave < 6) 
		octave++;
	    if (button_pressed(BTN_DOWN) && octave > 0) 
		octave--;
	    change = 1;
	}
    }

    else if (button_pressed(BTN_SET)) {
	insert_pattern_note(current_channel, current_pos, octave, note, length);
	state = STATE_PROGRAM;
    }

    else {
	leds_7seg_set(4, length);
	if (button_pressed(BTN_UP) || button_pressed(BTN_DOWN)) {
	    if (button_pressed(BTN_UP) && length < 4) 
		length++;
	    if (button_pressed(BTN_DOWN) && length > 1)
		length--;
	    change = 1;
	}

	for (uint8_t i = 8; i < 24; i++) {
	    if (button_pressed(i)) {
		if (switch_to_note(i) != 0xFF) {
		    change = 1;
		    note = switch_to_note(i);
		}
	    }
	}
    }

    if (change) 
	play_note(current_channel, pack_note_value(octave, note, length), cnt); 
}

static inline void program()
{

    if (button_pressed(BTN_PLAY)) {
	state = STATE_PLAY;
	current_pos = 0;
	counter = cnt;
    }
    
    if (button_pressed(BTN_SET)) 
	state = STATE_TOPLEVEL;

    if (button_pressed(BTN_UP) && current_channel < 4) 
	current_channel++;
    
    else if (button_pressed(BTN_DOWN) && current_channel > 0)
	current_channel--;
    
    for (uint8_t i = 8; i < 24; i++) {
	if (button_pressed(i)) {	    
	    state = STATE_WAIT_NOTE;
	    current_pos = btn_to_pos(i);
	    button_leds[i] = 1; // blink the corresponding LED
	    break;
	}
    }
}

static void play_note(uint8_t channel, uint16_t value, uint8_t count)
{
    if (value == 0) 
	return;

    uint8_t octave = note_octave(value);
    uint8_t note = note_value(value);
    uint8_t length = note_length(value);
    
    uint16_t period = note_to_period(octave, note);
	
    switch (channel) {
    case CHN_SQ1:
	env1.gate = 1;
	periods[0] = period;
	sq1_counter = length * count / 4;
	break;
	
    case CHN_SQ2:
	env2.gate = 1;
	periods[1] = period;
	sq2_counter = length * count / 4;
	break;
	
    case CHN_TRI:
	periods[2] = period;
	tri_counter = length * count / 4;
	break;

    case CHN_NOISE:
	env3.gate = 1;
	periods[3] = (octave << 4) | note;
	noise_counter = length * count / 4;
	break;

    case CHN_DMC:
	sample_load(&dmc.sample, note);
	if (dmc.sample.size != 0)
	    dmc.sample_enabled = 1;
	dmc.sample_loop = 0;
	break;
    }
}

void sequence_handler()
{
    if (sq1_counter > 0) {
	if (--sq1_counter == 0) 
	    env1.gate = 0;
    }

    if (sq2_counter > 0) {
	if (--sq2_counter == 0)
	    env2.gate = 0;
    }

    if (tri_counter > 0) {
	if (--tri_counter == 0)
	    periods[2] = 0;
    }

    if (noise_counter > 0) {
	if (--noise_counter == 0)
	    env3.gate = 0;
    }

    if (state == STATE_PLAY) {
	if (counter == cnt) {
	    for (uint8_t i = CHN_SQ1; i <= CHN_DMC; i++) {
		uint16_t note_value = patterns[i][current_pos];
		play_note(i, note_value, cnt);
	    }
	    if (++current_pos == 16) current_pos = 0;
	    counter = 0;
	}
	counter++;    
    }
}

static inline void play()
{
    if (button_pressed(BTN_PLAY)) {
	state = STATE_PROGRAM;
	env3.gate = 0;
	env2.gate = 0;
	env1.gate = 0;
	periods[2] = 0;	
    }
}

void load_pattern(uint8_t pattern_number)
{
    // patterns are stored right after patch memory
    uint16_t memory_pointer = PATCH_MEMORY_END + 5 * 32 * pattern_number;

    for (uint8_t i = 0; i < 5; i++) {
	for (uint8_t j = 0; j < 16; j++) {
	    patterns[i][j] = memory_read(memory_pointer++);
	    patterns[i][j] |= (memory_read(memory_pointer++)) << 8;
	}
    }
}

void save_pattern(uint8_t pattern_number)
{
    uint16_t memory_pointer = PATCH_MEMORY_END + 5 * 32 * pattern_number;

    for (uint8_t i = 0; i < 5; i++) {
	for (uint8_t j = 0; j < 16; j++) {
	    memory_write(memory_pointer++, patterns[i][j] & 0xFF);
	    memory_write(memory_pointer++, (patterns[i][j] >> 8) & 0xFF);
	}
    }   
}

static inline void toplevel()
/*
  Pattern programming toplevel handler

  Changing of patterns and selecting of channel to edit pattern for.
*/
{
    static uint8_t save_flag = 0;

    if (button_pressed(BTN_SAVE)) {
	save_flag ^= 1;
	button_leds[BTN_SAVE] = save_flag;
	if (!save_flag)
	    save_pattern(current_pattern);
    }

    // Has the current pattern number been changed? In that case, load the new patterns into RAM
    uint8_t last_pattern = current_pattern;
    updown(&current_pattern, 0, 99);
    if (current_pattern != last_pattern) {
	if (!save_flag) 
	    load_pattern(current_pattern);
    }
	    
    if (button_pressed(BTN_SET)) 
	state = STATE_PROGRAM;
 
}

void sequencer()
{
    //   sequence_handler();

    switch (state) {
    case STATE_TOPLEVEL:
	toplevel(); break;
    case STATE_WAIT_NOTE:
	wait_note(); break;
    case STATE_PROGRAM:
	program(); break;
    case STATE_PLAY:
	play(); break;
    }

    // Put values in upper 16 button LEDs
    for (uint8_t i = 8; i < 24; i++) {
	switch (state) {
	case STATE_PROGRAM:
	    leds_7seg_clear(4);
	    leds_7seg_set(3, current_channel + 1);
	    button_leds[i] = (patterns[current_channel][btn_to_pos(i)] != 0) * 0xFF;
	    break;

	case STATE_WAIT_NOTE:
	    leds_7seg_set(3, current_channel + 1);
	    // Skip the current position led, since it is set to blink
	    if (i != pos_to_btn(current_pos))
		button_leds[i] = (patterns[current_channel][btn_to_pos(i)] != 0) * 0xFF;
	    break;

	case STATE_PLAY:
	    leds_7seg_two_digit_set(3, 4, current_pattern);
	    button_leds[i] = (i == pos_to_btn(current_pos)) * 0xFF;
	    break;

	case STATE_TOPLEVEL:
	    leds_7seg_two_digit_set(3, 4, current_pattern);
	    break;
	}
    }
}
