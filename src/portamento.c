#include <avr/io.h>
#include "portamento.h"
#include "modulation.h"

static uint8_t counters[3] = {0};
uint8_t portamento_values[3] = {0};
uint16_t portamento_targets[3] = {0};

static inline int16_t abs(int16_t a)
{
    return (a >= 0) ? a : -a;
}

static inline int16_t dc_to_dT(uint16_t period, int16_t dc)
/* Takes an adjusted frequency delta (5.5 bit format) and computes the corresponding
   period delta */
{
    int16_t temp = dc * ((int32_t)2005 * (int32_t)period / 104167);
    temp /= 32;    // 5 bits were used as fractional part
    return temp;
}

static int16_t dc = 16;

void portamento_handler()
{
    for (uint8_t i = 0; i < 3; i++) {
	if (portamento_values[i] == 0) 
	    mod_periods[i] = portamento_targets[i];
	else {
	    counters[i]++;
	    if (counters[i] == portamento_values[i]) {
		counters[i] = 0;
		if (abs(mod_periods[i] - portamento_targets[i]) < abs(dc_to_dT(mod_periods[i], dc))) 
		    mod_periods[i] = portamento_targets[i];
		else if (portamento_targets[i] <= 2005 && portamento_targets[i] >= 49) {
		    if (portamento_targets[i] > mod_periods[i]) 
			mod_periods[i] += dc_to_dT(mod_periods[i], dc);
		    else
			mod_periods[i] += dc_to_dT(mod_periods[i], -dc);
		}
	    }
	}
    }
}
