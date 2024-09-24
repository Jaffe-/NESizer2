/*
  Copyright 2024 Johan Fjeldtvedt and Beau Sterling

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



  Note Stack

  Buffers held notes in monophonic mode, to improve live performace experience.
  This behavior is called "legato" mode in most synths.
  In polyphonic mode, or DCM channel, reverts to previous behavior.
*/

#include <stdint.h>

void note_stack_push(uint8_t channel, uint8_t note);
void note_stack_pop(uint8_t channel, uint8_t note);
