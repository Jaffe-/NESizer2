#include <avr/io.h>
#include <avr/pgmspace.h>
#include "modulation.h"
#include "portamento.h"
#include "apu.h"
//#include "lfo.h"
#include "envelope.h"
#include "assigner.h"

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
#define A4 236
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

uint16_t note_to_period(uint8_t channel, uint8_t note)
/* 
   Takes a note in MIDI note format and returns its corresponding period 
   
   The channel parameter is used to adjust the period for the triangle channel
*/
{
    uint8_t note_num = note % 12;
    uint8_t octave = (note - note_num) / 12 - 2;

    if (channel == CHN_TRI) 
	octave += 1;

    return pgm_read_word(&period_table[octave][note_num]);
}

static uint8_t notes[5];

void play_note(uint8_t channel, uint8_t note)
{
    //   notes[channel] = note;

    uint16_t period = note_to_period(channel, note);

    switch (channel) {
    case CHN_SQ1:
	env1.gate = 1;
	portamento_targets[0] = period;
	break;
	
    case CHN_SQ2:
	env2.gate = 1;
	portamento_targets[1] = period;
	break;
	
    case CHN_TRI:
	tri.silenced = 0;
	if (get_envmod(CHN_TRI) != 0)
	    env3.gate = 1;
	portamento_targets[2] = period;
	break;

    case CHN_NOISE:
	env3.gate = 1;
	mod_periods[3] = note - 24;
	break;

    case CHN_DMC:
	sample_load(&dmc.sample, note - 24);
	if (dmc.sample.size != 0)
	    dmc.sample_enabled = 1;
	dmc.sample_loop = 0;
	break;
    }
}

void stop_note(uint8_t channel)
{
    switch (channel) {
    case CHN_SQ1:
	env1.gate = 0;
	break;
	
    case CHN_SQ2:
	env2.gate = 0;
	break;
	
    case CHN_TRI:
	tri.silenced = 1;
	break;

    case CHN_NOISE:
	env3.gate = 0;
	break;

    case CHN_DMC:
	dmc.sample_enabled = 0;
    }
}

void assigner_handler()
{
    
}
