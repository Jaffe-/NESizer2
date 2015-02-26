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

Task tasks[14] = {
  {.handler = &apu_dmc_update_handler, .period = 1, .counter = 1},
  {&midi_io_handler, 5, 0},
  {&apu_update_handler, 10, 1},
  {&lfo_update_handler, 10, 2},
  {&envelope_update_handler, 10, 3},
  {&portamento_handler, 10, 4},
  {&midi_handler, 10, 5},
  {&mod_calculate, 10, 6},
  {&mod_apply, 10, 7},
  {&leds_refresh, 20, 8},
  {&input_refresh, 80, 8},
  {&sequence_handler, 80, 9},
  {&ui_handler, 80, 9},
  {&ui_leds_handler, 80, 9}
};

static const uint8_t num_tasks = 14;

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
