/*
  Copyright 2014-2015 Johan Fjeldtvedt 

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



  main.c - Main entry point

  Entry point for the program. Initializes the system and starts the
  task handler.
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
