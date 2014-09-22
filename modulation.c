#include <avr/io.h>
#include <avr/pgmspace.h>
#include "modulation.h"
#include "apu.h"
#include "lfo.h"
#include "envelope.h"

uint16_t mod_periods[4] = {0};
uint8_t mod_lfo_modmatrix[4][3] = {{0}};
uint8_t mod_detune[3] = {9, 9, 9};   // detune values default to 9 (translates to 0)
uint8_t mod_envmod[4] = {9, 9, 9, 7};

static inline int16_t dc_to_dT(uint16_t period, int16_t dc)
/* Takes an adjusted frequency delta (5.5 bit format) and computes the corresponding
   period delta */
{
    int16_t temp = -dc * ((int32_t)2005 * (int32_t)period / 104167);
    temp /= 32;    // 5 bits were used as fractional part
    return temp;
}

static uint16_t* period_ptrs[] = {&(sq1.period), &(sq2.period), &(tri.period)};
static Envelope* envelopes[] = {&env1, &env2, &env3};

static inline int8_t get_detune(uint8_t chn)
{
    return (int8_t)mod_detune[chn] - 9;
}

int8_t get_envmod(uint8_t chn)
{
    if (chn == CHN_NOISE)
	return (int8_t)mod_envmod[2] - 7;
    else 
	return (int8_t)mod_envmod[chn] - 9;
}

static inline void apply_freqmod()
{
    // Define some helper arrays
    static LFO* lfos[] = {&lfo1, &lfo2, &lfo3};
    static uint8_t i = 0;

    uint8_t* intensities = mod_lfo_modmatrix[i];
    int16_t sum = 0;
    uint8_t c_sum = 0;
    uint8_t cnt = 0;

    // TODO: fix this ugly mess
    // LFO mix:
    for (uint8_t j = 0; j < 3; j++) { 
	if (intensities[j] > 0) {
	    cnt++;
	    c_sum += intensities[j];
	    sum += intensities[j] * lfos[j]->value;
	}
    }

    if (cnt > 0) 
	sum /= c_sum;
    
    if (i <= CHN_TRI) {
	// Frequency delta due to LFO
	int16_t dc = (sum * c_sum / cnt) / 64;

	// Add detune frequency delta
	dc += get_detune(i);

	// For square channels, also add in the envelope modulation, if any
	int8_t env_fmod_val = (int8_t)envelopes[i]->value - (int8_t)envelopes[i]->sustain;
	if (env_fmod_val > 0) 
	    dc += 2 * env_fmod_val * get_envmod(i);
	
	// Convert frequency delta to a period delta and add to the base period
	*(period_ptrs[i]) = mod_periods[i] + dc_to_dT(mod_periods[i], dc);
    }
    else {
	noise.period = mod_periods[i] + sum / 32 + (int8_t)env3.value * ((int8_t)mod_envmod[i] - 7);
    }
    
    if (++i == 4) i = 0;
}

static inline void apply_envelopes()
{
    sq1.volume = env1.value;
    sq2.volume = env2.value;
    noise.volume = env3.value;    
}

void modulation_handler() 
{
    apply_freqmod(); 
    apply_envelopes(); 
}
