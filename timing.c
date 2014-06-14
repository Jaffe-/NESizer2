#include "constants.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "timing.h"
#include "2a03_io.h"
#include "apu.h"
#include "envelope.h"
#include "lfo.h"
#include "leds.h"
#include "kick.c"
#include "snare.c"

enum PatternCommands {DRUM_KICK, DRUM_SNARE, SILENCE, EMPTY, END};

typedef struct {
    uint8_t* dmc_data;
    uint8_t* noise_data;
    uint8_t current;
} DrumPattern;

DrumPattern dp;

void drum_pattern_update(DrumPattern* pattern);

uint16_t sq1_note;
uint16_t sq2_note;
uint16_t tri_note;

uint8_t time_step;
uint16_t testp;

void setup_timer()
{
/* Sets up the timing interrupt 

Timer triggers inerrupt at X kHz
*/
    time_step = 0;

    // Enable interrupts
    sei();
    
    // CTC operation
    TCCR0A = 0b10;

    OCR0A = 200;
//    TCNT0 = 0;

    // Enable compare match interrupt
    TIMSK0 = 0b10;

    // Normal operation, clock prescaler 1024
    TCCR0B |= 0b10000010;
    
}

ISR(TIMER0_COMPA_vect)
/* 
   16kHz (16025 Hz) timer interrupt which drives all the logic
 */
{
    if (dmc.enabled && dmc.sample_enabled) 
	dmc_update_sample();
    
    testp++;

    if (testp % 2 == 0) {
	lfo_update(&lfo1);
	lfo_update(&lfo2);
    }

    if (testp % 40 == 0) {
	envelope_update(&env1);
	envelope_update(&env2);
	noise.volume = env1.value >> 1;
	sq1.volume = env2.value >> 1;
	sq2.volume = env2.value >> 1;

	sq1.period = sq1_note;
	sq2.period = sq2_note;
	tri.period = tri_note;
	lfo_apply_square(&lfo2, &sq2, 20);
	lfo_apply_square(&lfo1, &sq1, 20);

	sq1_update();
	sq2_update();
	tri_update();
	noise_update();
	apu_refresh();
    }

    if (testp == 2000) {
	drum_pattern_update(&dp);
	testp = 0;
    }
    
}

// test
void drum_pattern_update(DrumPattern* pattern) 
{
    switch (pattern->dmc_data[pattern->current]) {
    case DRUM_KICK:
	dmc.sample = kick_raw;
	dmc.sample_length = kick_raw_len;
	dmc.sample_enabled = 1;
	dmc.current = 0;
	break;
    case DRUM_SNARE:
	dmc.sample = snare_raw;
	dmc.sample_length = snare_raw_len;
	dmc.sample_enabled = 1;
	dmc.current = 0;
	break;
    case SILENCE:
	dmc.sample_enabled = 0;
	break;
    case EMPTY:
	break;
    case END:
	pattern->current = 0;	
    }
    
    if (pattern->noise_data[pattern->current] == 1) {
	env1.gate = 1;
	env1.gate_prev = 0;
    } 

    tri_note = basspat[pattern->current] << 2;
    sq1_note = sq1_pat[pattern->current] << 1;
    sq2_note = sq2_pat[pattern->current] << 1;
    env2.gate = sq_mask[pattern->current];
    
    set_leds(1 << (pattern->current >> 1));

    pattern->current++;

    if (pattern->current == 16) 
	pattern->current = 0;

}
