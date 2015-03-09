#include <avr/io.h>
#include "portamento.h"
#include "modulation.h"

uint8_t portamento_values[3] = {0};
uint16_t portamento_cs[3] = {0};
uint8_t portamento_target_notes[3] = {0};

static uint8_t counters[3] = {0};

static inline uint16_t note_to_c(uint8_t note)
{
  return note << 4;
}

void portamento_handler()
{
  for (uint8_t i = 0; i < 3; i++) {
    if (portamento_values[i] == 0)
      portamento_cs[i] = (uint16_t)portamento_target_notes[i] << 6;
    else {
      if (++counters[i] == portamento_values[i]) {
	counters[i] = 0;
	if (portamento_cs[i] > note_to_c(portamento_target_notes[i]))
	  portamento_cs[i]--;
	else if (portamento_cs[i] < note_to_c(portamento_target_notes[i]))
	  portamento_cs[i]++;
      }
    }
  }
}

