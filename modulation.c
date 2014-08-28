#include <avr/io.h>
#include "modulation.h"
#include "apu.h"
#include "lfo.h"
#include "envelope.h"

uint16_t bperiods[4] = {0};
uint8_t lfo_mod_matrix[4][3] = {{0}};

void apply_lfos()
{
    // Define some helper arrays
    LFO* lfos[] = {&lfo1, &lfo2, &lfo3};
    uint16_t* periods[] = {&(sq1.period), &(sq2.period), &(tri.period)};

    for (uint8_t i = 0; i < 4; i++) {
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
	    *(periods[i]) = bperiods[i] + sum;
	else
	    noise.period = bperiods[i] + (sum >> 4);
    }
}

void apply_envelopes()
{
    sq1.volume = env1.value;
    sq2.volume = env2.value;
    noise.volume = env3.value;
}

void modulation_handler() 
{
    apply_lfos();
    apply_envelopes();
    sq1_update();
    sq2_update();
    tri_update();
    noise_update();
}
