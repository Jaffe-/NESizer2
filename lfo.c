#include <avr/io.h>
#include <avr/pgmspace.h>
#include "lfo.h"
#include "apu.h" 
#include "sine.c"

LFO lfo1 = {0};
LFO lfo2 = {0};
LFO lfo3 = {0};

static uint8_t A4_period = 237;

void lfo_update(LFO* lfo)
{
    lfo->counter++;
    if (lfo->counter == lfo->period) {
	lfo->counter = 0;

	switch (lfo->waveform) {
	case LFO_SINE:
	    lfo->value = pgm_read_byte(&sine_table[lfo->position]) - 0x80;
	    break;
	case LFO_RAMP_DOWN:
	    lfo->value = lfo->position - 0x80;
	    break;
	case LFO_RAMP_UP:
	    lfo->value = 0x80 - lfo->position;
	    break;
	}

	lfo->position += 4;
    }
}

int16_t lfo_value(LFO* lfo, uint8_t intensity)
{
    if (intensity == 0) 
	return 0;
    else
	return lfo->value;
}


