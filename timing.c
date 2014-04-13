#include "constants.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "timing.h"
#include "2a03_io.h"
#include "apu.h"
#include "envelope.h"
#include "lfo.h"
#include "leds.h"

uint8_t time_step;
uint16_t testp;

void setup_timer()
{
    time_step = 0;
    testp = 800;

    // Enable interrupts
    sei();
    
    // CTC operation
    TCCR0A = 0b10;

    // Normal operation, clock prescaler 1024
    TCCR0B = 0b10000010;
    
    OCR0A = 156;
    OCR0B = 0;

    // Enable compare match interrupt
    TIMSK0 = 0b10;
}

ISR(TIMER0_COMPA_vect)
/* 
   16kHz (16025 Hz) timer interrupt which drives all the logic
 */
{

    // Update sample in DMC
    if (dmc.enabled && dmc.sample_enabled) 
	dmc_update_sample();
  
    // Refresh APU registers
    apu_refresh();

    // Envelopes are updated at 250 Hz frequency
    if (time_step++ == 64) 
	time_step = 0;
    if (time_step == 1) {
	envelope_update(&env1);
	envelope_update(&env2);
	envelope_update(&env3);
    }
    if (time_step % 2 == 1) {
	lfo_update(&lfo1);
	lfo_update(&lfo2);
	lfo_update(&lfo3);
    }
    if (time_step % 2 == 0) {
//	sq2.volume = env2.value;
	lfo_apply_square(&sq2, &lfo1);
	lfo_apply_square(&sq1, &lfo2);
//	lfo_apply_triangle(&tri, &lfo1);
	sq2_update();
	sq1_update();
	set_leds(1 << (((uint8_t)(lfo1.value + 0x80) >> 5)));
    }
}
