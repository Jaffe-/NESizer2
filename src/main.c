/*
  NESIZER
  Entry point 

  (c) Johan Fjeldtvedt

  Contains the main function, which sets up the various subsystems and transfers
  control to a task handler.
 */

#define F_CPU 20000000L

#include <avr/io.h>
#include <util/delay.h>
#include "task.h"
#include "apu.h"
#include "2a03_io.h"
#include "leds.h"
#include "memory.h"
#include "bus.h"
#include "patch.h"
#include "midi_io.h"

int main() 
{
  // Set up low level systems:
  bus_setup();
  if (io_setup()) {
    memory_setup();
    task_setup();
    midi_io_setup();
    apu_setup();
  
    // Load first patch
    patch_load(0);
    
    // The task manager takes over from here
    task_manager();
  }

  // If 2A03 setup failed, display the clock divisor for debugging purposes
  else {
    leds_7seg_two_digit_set(3, 4, io_clockdiv);
    leds[0] = 0;
    leds[1] = 0;
    leds[2] = 0;
    while (1) {
      leds_refresh();
      _delay_ms(2);
    }
  }
}
