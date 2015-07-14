#include <avr/io.h>
#include <avr/interrupt.h>
#include "task.h"
#include "apu.h"
#include "midi_io.h"
#include "lfo.h"
#include "envelope.h"
#include "modulation.h"
#include "portamento.h"
#include "midi.h"
#include "leds.h"
#include "input.h"
#include "ui.h"
#include "ui_sequencer.h"

Task tasks[] = {
  {.handler = &apu_dmc_update_handler, .period = 1, .counter = 1},
  {.handler = &midi_io_handler, .period = 5, .counter = 0},
  {.handler = &apu_update_handler, .period = 10, .counter = 1},
  {.handler = &lfo_update_handler, .period = 10, .counter = 2},
  {.handler = &envelope_update_handler, .period = 10, .counter = 3},
  {.handler = &portamento_handler, .period = 10, .counter = 4},
  {.handler = &midi_handler, .period = 10, .counter = 5},
  {.handler = &mod_calculate, .period = 10, .counter = 6},
  {.handler = &mod_apply, .period = 10, .counter = 7},
  {.handler = &leds_refresh, .period = 20, .counter = 8},
  {.handler = &input_refresh, .period = 80, .counter = 8},
  {.handler = &sequence_handler, .period = 80, .counter = 9},
  {.handler = &ui_handler, .period = 80, .counter = 9},
  {.handler = &ui_leds_handler, .period = 80, .counter = 9}
};

static const uint8_t num_tasks = sizeof(tasks)/sizeof(Task);

static volatile uint8_t ticks;

void task_setup()
{
/* 
   Sets up the timing interrupt to occur at 16 kHz
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

void task_stop()
{
  TIMSK0 = 0;
}

void task_restart()
{
  TIMSK0 = 1 << OCIE0A;
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
    // Wait until new tick arrives
    while (ticks == last_tick);

    // Loop through each task and update its count
    for (uint8_t i = 0; i < num_tasks; i++)
      tasks[i].counter += ticks - last_tick;

    last_tick = ticks;

    for (uint8_t i = 0; i < num_tasks; i++) {
      // If number of ticks changed after previous task, stop going further
      if (ticks != last_tick)
	break;

      // Call the task handler when the count reaches the period
      if (tasks[i].counter >= tasks[i].period) {
	tasks[i].counter = 0;
	tasks[i].handler();
      }
    }
  }
}

