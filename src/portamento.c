#include <avr/io.h>
#include "portamento.h"
#include "modulation.h"

uint8_t portamento_values[3] = {0};
uint16_t portamento_periods[3] = {0};
int16_t portamento_dcs[3] = {0};
int8_t portamento_target_notes[3] = {0};

static int8_t old_notes[3] = {0};
static uint8_t counters[3] = {0};

static const uint16_t d_dc = 1;

void portamento_handler()
{
  for (uint8_t i = 0; i < 3; i++) {
    mod_periods[i] = portamento_periods[i];
    
    if (portamento_values[i] > 0) {
      if (portamento_target_notes[i] != old_notes[i]) {
	portamento_dcs[i] = 16 * (old_notes[i] - portamento_target_notes[i]) + portamento_dcs[i];
	old_notes[i] = portamento_target_notes[i];
      }
      
      if (++counters[i] == portamento_values[i]) {
	counters[i] = 0;
	
	if (portamento_dcs[i] < 0)
	  portamento_dcs[i] += d_dc;
	else if (portamento_dcs[i] > 0)
	  portamento_dcs[i] -= d_dc;
      }
    }
  }
}
