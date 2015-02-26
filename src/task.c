#include <avr/io.h>
#include <avr/interrupt.h>
#include "task.h"
#include "timing.h"
#include "apu.h"

Task tasks[16];
static uint8_t num_tasks = 0;

static volatile uint8_t ticks;

void task_setup()
{
/* Sets up the timing interrupt 

Timer triggers inerrupt at 16 kHz
*/
    // Enable interrupts
    sei();
    
    // CTC operation
    TCCR0A = 0b10 << WGM00;

    // Set period (
    OCR0A = 156;

    // Enable compare match interrupt
    TIMSK0 = 1 << OCIE0A;

    // Normal operation, clock prescaler 8
    TCCR0B = (1 << FOC0A) | (0b010 << CS00); 
}

ISR(TIMER0_COMPA_vect)
/* 
   16kHz (16025 Hz) timer interrupt
 */
{
    ticks++;
}

void task_manager()
{
  uint8_t last_tick = 0;

  while(1) {
    // wait until new tick arrives
    while (ticks == last_tick);

    for (uint8_t i = 0; i < num_tasks; i++)
      tasks[i].counter += ticks - last_tick;

    last_tick = ticks;

    for (uint8_t i = 0; i < num_tasks; i++) {
      if (tasks[i].counter >= tasks[i].period) {
	tasks[i].counter = 0;
	tasks[i].handler();
	break;
      }
    }
  }
}

void task_add(void (*handler)(), uint8_t period, uint8_t offset)
{
    tasks[num_tasks].handler = handler;
    tasks[num_tasks].period = period;
    tasks[num_tasks].counter = 0;
    num_tasks++;
}
