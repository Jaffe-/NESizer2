/*
  NESIZER
  Entry point 

  (c) Johan Fjeldtvedt

  Contains the main function, which sets up the various subsystems and transfers
  control to a task handler.
 */

#include "task.h"
#include "apu.h"
#include "2a03_io.h"
#include "memory.h"
#include "bus.h"
#include "patch.h"
#include "midi_io.h"
#include "periods.h"

int main() 
{
  // Set up low level systems:
  bus_setup();
  io_setup();
  periods_setup();
  memory_setup();
  task_setup();
  midi_io_setup();
  apu_setup();
  
  // Load first patch
  patch_load(0);
  
  // The task manager takes over from here
  task_manager();
}
