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



  Main entry point

  Entry point for the program. Initializes the system and starts the
  task/task.handler.
*/


#include "task/task.h"
#include "apu/apu.h"
#include "io/2a03.h"
#include "io/memory.h"
#include "io/bus.h"
#include "patch/patch.h"
#include "io/midi.h"
#include "io/battery.h"
#include "modulation/periods.h"

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
    battery_setup();

    // Load first patch
    patch_load(0);

    // The task manager takes over from here
    task_manager();
}
