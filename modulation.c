#include <avr/io.h>
#include "modulation.h"
#include "apu.h"
#include "lfo.h"
#include "envelope.h"

uint16_t periods[4] = {0};
uint8_t lfo_mod_matrix[4][3] = {{0}};
uint8_t detune[3] = {9, 9, 9};   // detune values default to 9 (translates to 0)

static inline void apply_lfos()
{
    // Define some helper arrays
    static const LFO* lfos[] = {&lfo1, &lfo2, &lfo3};
    static uint16_t* period_ptrs[] = {&(sq1.period), &(sq2.period), &(tri.period)};
    static uint8_t i = 0;

    uint8_t* intensities = lfo_mod_matrix[i];
    int16_t sum = 0;
    uint8_t cnt = 0;

    for (uint8_t j = 0; j < 3; j++) { 
	if (intensities[j] > 0) {
	    cnt++;
	    sum += lfo_value(lfos[j], intensities[j]);
	}
    }

    if (cnt > 0) 
	sum /= cnt;
    
    if (i < 3) 
	*(period_ptrs[i]) = periods[i] + ((int16_t)detune[i] - 9) + sum;
    else
	noise.period = periods[i] + (sum >> 4);
    
    if (i++ == 4) i = 0;
}

static inline void apply_envelopes()
{
    sq1.volume = env1.value;
    sq2.volume = env2.value;
    noise.volume = env3.value;
}

void modulation_handler() 
{
    apply_lfos(); 
    apply_envelopes(); 
}
