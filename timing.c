#include "constants.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "timing.h"
#include "2a03_io.h"
#include "apu.h"
#include "envelope.h"
#include "lfo.h"
#include "leds.h"
#include "drums.h"

uint8_t time_step;
uint16_t testp;

void setup_timer()
{
/* Sets up the timing interrupt 

Timer triggers inerrupt at X kHz
*/
    time_step = 0;

    // Enable interrupts
    sei();
    
    // CTC operation
    TCCR0A = 0b10;

    OCR0A = 200;
//    TCNT0 = 0;

    // Enable compare match interrupt
    TIMSK0 = 0b10;

    // Normal operation, clock prescaler 1024
    TCCR0B |= 0b10000010;
    
}

ISR(TIMER0_COMPA_vect)
/* 
   16kHz (16025 Hz) timer interrupt which drives all the logic
 */
{
    if (dmc.enabled && dmc.sample_enabled) 
	dmc_update_sample();
    
    testp++;

    if (testp % 40 == 0) {
	envelope_update(&env1);
	envelope_update(&env2);
	lfo_update(&lfo1);
	noise.volume = env1.value >> 1;
	sq1.volume = env2.value >> 1;
	sq2.volume = env2.value >> 1;
	
	lfo_apply_square(&lfo1, &sq1, 10);
	
	sq1_update();
	sq2_update();
	noise_update();
	apu_refresh();
    }

    if (testp == 2000) {
	drum_pattern_update(&dp);
	testp = 0;
    }
    
}
