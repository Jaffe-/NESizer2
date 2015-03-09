#include <stdint.h>
#include <avr/pgmspace.h>

extern uint8_t note_min;
extern const uint8_t note_max;

uint16_t get_period(uint8_t chn, uint16_t c);
void periods_setup();
