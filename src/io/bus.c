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



  Bus interface

  Handles low level bus communication.
*/


#include <avr/io.h>
#include "io/bus.h"

void bus_setup(void)
{
    // Set address related port pins as outputs:
    DDRB |= ADDR_m | BUS_EN_m;

    // The bus_dir_output macro will take care of setting the databus pins
    // correctly:
    bus_dir_output();
}
