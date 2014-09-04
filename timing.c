#include "constants.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "timing.h"
#include "2a03_io.h"
#include "apu.h"
#include "envelope.h"
#include "lfo.h"
#include "leds.h"

void timer_setup()
{
/* Sets up the timing interrupt 

Timer triggers inerrupt at X kHz
*/
    // Enable interrupts
    sei();
    
    // CTC operation
    TCCR0A = 0b10 << WGM00;

    // Set period
    //OCR0A = F_CPU / (8.0 * F_INTERRUPT);
    OCR0A = 156;

    // Enable compare match interrupt
    TIMSK0 = 1 << OCIE0A;

    // Normal operation, clock prescaler 8
//    TCCR0B |= 0b10000010;
    TCCR0B = (1 << FOC0A) | (0b010 << CS00); 
}

ISR(TIMER0_COMPA_vect)
/* 
   16kHz (16025 Hz) timer interrupt which drives all the logic
 */
{
    ticks++;
//    if (dmc.enabled && dmc.sample_enabled) 
//	dmc_update_sample();

}

