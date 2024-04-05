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
#include "assigner/assigner.h"
#include "sequencer/sequencer.h"
#include "ui/ui_sequencer.h"
#include "ui/ui_programmer.h"
#include "ui/ui.h"
#include "settings/settings.h"
#include "patch/patch.h"

#include <util/delay.h>

#define MAGIC 0xdeadbeef
#define MAGIC_ADDR 0

static bool ram_integrity_check(void)
{
    for (uint8_t i = 0; i < 8; i++)
        if (memory_read_dword(MAGIC_ADDR+4*i) != MAGIC)
            return false;
    return true;
}

static void ram_initialize(void)
{
    for (uint8_t i = 0; i < 8; i++)
        memory_write_dword(MAGIC_ADDR+4*i, MAGIC);
    settings_init();
    for (uint8_t i = 0; i < 100; i++)
        patch_initialize(i);
    sequencer_pattern_init();
}

void startup_check(void)
{
    /* This delay is necessary to get a stable and correct voltage measurement */
    bool battery_good = battery_read() > 25;
    bool ram_good = ram_integrity_check();
    ui_startup_errors =
        ((battery_good ? 0 : 1) << UI_STARTUP_ERROR_BATTERY_LOW)
        | ((ram_good ? 0 : 1) << UI_STARTUP_ERROR_CORRUPT_RAM);

    if (!ram_good) {
        ram_initialize();
    }
}

int main()
{
    // Set up low level:
    bus_setup();
    memory_setup();
    io_setup();
    midi_io_setup();
    apu_setup();
    battery_setup();

    startup_check();

    // Set up higher level:
    task_setup();
    assigner_setup();
    periods_setup();
    sequencer_setup();
    ui_sequencer_setup();
    ui_programmer_setup();

    // The task manager takes over from here
    task_manager();
}
