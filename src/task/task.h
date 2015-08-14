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



  Task handler

  Loops through a pre-defined list of tasks and executes them at 
  their set intervals. 
*/


#pragma once

#include <stdint.h>

struct task {
  void (*const handler)();
  const uint8_t period;
  uint8_t counter;
};

void task_add(void (*handler)(), uint8_t period, uint8_t offset);
void task_manager(void);
void task_setup(void);
