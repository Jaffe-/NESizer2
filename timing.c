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
    TCCR0A = 0b10;

    OCR0A = F_CPU / (8.0 * F_INTERRUPT);

    // Enable compare match interrupt
    TIMSK0 = 0b10;

    // Normal operation, clock prescaler 8
    TCCR0B |= 0b10000010;
    
}

ISR(TIMER0_COMPA_vect)
/* 
   16kHz (16025 Hz) timer interrupt which drives all the logic
 */
{
    ticks++;
}

