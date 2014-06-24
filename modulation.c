#include <avr/io.h>
#include "modulation.h"
#include "apu.h"
#include "lfo.h"

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

	*(periods[i]) += sum;
    }
}

void modulation_handler() 
{
    apply_lfos();
}
