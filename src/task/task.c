/*
  Copyright 2014-2016 Johan Fjeldtvedt

  This file is part of NESIZER.

  NESIZER is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  NESIZER is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NESIZER.  If not, see <http://www.gnu.org/licenses/>.



  Task handler

  Loops through a pre-defined list of tasks and executes them at
  their set intervals.
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "task.h"
#include "apu/apu.h"
#include "io/midi.h"
#include "lfo/lfo.h"
#include "envelope/envelope.h"
#include "modulation/modulation.h"
#include "portamento/portamento.h"
#include "midi/midi.h"
#include "io/leds.h"
#include "io/input.h"
#include "ui/ui.h"
#include "ui/ui_sequencer.h"
#include "assigner/assigner.h"
#include "sequencer/sequencer.h"

struct task {
    void (*const handler)(void);
    const uint8_t period;
    uint8_t counter;
};

struct task tasks[] = {
    {.handler = &apu_dmc_update_handler, .period = 1, .counter = 1},
    {.handler = &lfo_update_handler, .period = 1, .counter = 1},
    {.handler = &midi_io_handler, .period = 5, .counter = 0},
    {.handler = &apu_update_handler, .period = 10, .counter = 1},
    {.handler = &envelope_update_handler, .period = 10, .counter = 3},
    {.handler = &portamento_handler, .period = 10, .counter = 4},
    {.handler = &midi_handler, .period = 10, .counter = 5},
    {.handler = &mod_calculate, .period = 10, .counter = 6},
    {.handler = &mod_apply, .period = 10, .counter = 7},
    {.handler = &sequencer_handler, .period = 20, .counter = 5},
    {.handler = &leds_refresh, .period = 20, .counter = 8},
    {.handler = &input_refresh, .period = 80, .counter = 8},
    {.handler = &ui_handler, .period = 80, .counter = 9},
    {.handler = &ui_leds_handler, .period = 80, .counter = 9},
};

static const uint8_t num_tasks = sizeof(tasks)/sizeof(struct task);

static volatile uint8_t ticks;

void task_setup(void)
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

void task_stop(void)
{
    TIMSK0 = 0;
}

void task_restart(void)
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

void task_manager(void)
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

            // Call the task/task.handler when the count reaches the period
            if (tasks[i].counter >= tasks[i].period) {
                tasks[i].counter = 0;
                tasks[i].handler();
            }
        }
    }
}
