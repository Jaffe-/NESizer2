#include <avr/io.h>
#include <avr/pgmspace.h>
#include "constants.h"
#include "drums.h"
#include "apu.h"
#include "snare.c"
#include "kick.c"
#include "envelope.h"
#include "leds.h"

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

    tri.period = basspat[pattern->current] << 2;
    tri_update();

    sq1.period = sq1_pat[pattern->current] << 1;
    sq2.period = sq2_pat[pattern->current] << 1;
    env2.gate = sq_mask[pattern->current];
    
    set_leds(1 << (pattern->current >> 1));

    pattern->current++;

    if (pattern->current == 16) 
	pattern->current = 0;

}
