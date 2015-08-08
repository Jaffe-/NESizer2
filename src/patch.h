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



  Patch handling

  Handles storing and reading patches to/from memory.
*/


#pragma once

#include <stdint.h>

#define PATCH_MIN 0
#define PATCH_MAX 99

const uint16_t PATCH_MEMORY_END;

void patch_save(uint8_t num);
void patch_load(uint8_t num);
void patch_initialize(uint8_t num);
void patch_clean(void);
