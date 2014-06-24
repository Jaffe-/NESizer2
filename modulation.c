#include <avr/io.h>
#include "modulation.h"
#include "apu.h"
#include "lfo.h"
#include "envelope.h"

uint16_t bperiods[3] = {0};
uint8_t lfo_mod_matrix[3][3] = {{0}};
uint8_t env_mod_select[3] = {0};

void apply_lfos()
{
    // Define some helper arrays
    LFO* lfos[] = {&lfo1, &lfo2, &lfo3};
    uint16_t* periods[] = {&(sq1.period), &(sq2.period), &(tri.period)};

    for (uint8_t i = 0; i < 3; i++) {
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

	*(periods[i]) = bperiods[i] + sum;
    }
}

void apply_envelopes()
{
    Envelope* envelopes[] = {&env1, &env2, &env3};
    uint8_t* volumes[] = {&(sq1.volume), &(sq2.volume), &(noise.volume)};
    
    for (uint8_t i = 0; i < 3; i++) {
	Envelope* env = envelopes[env_mod_select[i]];
	*(volumes[i]) = env->value;
    }
}

void modulation_handler() 
{
    apply_lfos();
    apply_envelopes();
    sq1_update();
    sq2_update();
    tri_update();

}
