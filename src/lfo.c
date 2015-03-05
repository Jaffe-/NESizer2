#include "lfo.h"
#include <avr/pgmspace.h>
#include <stdint.h>
#include "apu.h" 
#include "data/sine.c"

LFO lfo1 = {0};
LFO lfo2 = {0};
LFO lfo3 = {0};

void lfo_update(LFO* lfo)
{
  if (++lfo->counter == lfo->period) {
    lfo->counter = 0;

    switch (lfo->waveform) {
    case SINE:
      lfo->value = pgm_read_byte_near(&sine_table[lfo->position]);
      break;
      
    case RAMP_DOWN:
      lfo->value = 4 * lfo->position - 0x80;
      break;

    case RAMP_UP:
      lfo->value = 0x80 - 4 * lfo->position;
      break;

    case SQUARE:
      if (lfo->position < 32)
	lfo->value = -127;
      else
	lfo->value = 127;
      break;

    case TRIANGLE:
      if (lfo->position < 32) 
	lfo->value = 0x80 - 8 * lfo->position;
      else
	lfo->value = 8 * (lfo->position - 32) - 0x80;
      break;

    default:
      break;
    }

    if (++lfo->position == 64)
      lfo->position = 0;
  }
}

int16_t lfo_value(LFO* lfo, uint8_t intensity)
{
  if (intensity == 0) 
    return 0;
  else
    return lfo->value;
}

void lfo_update_handler()
{  
  lfo_update(&lfo1);
  lfo_update(&lfo2);
  lfo_update(&lfo3);
}

