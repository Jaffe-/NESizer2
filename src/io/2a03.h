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



  Low level 2A03 I/O interface

  Contains functions for communicating with and controlling the
  2A03.
*/


#pragma once

#include <stdint.h>

#ifndef F_CPU
    #define F_CPU 20000000L
#endif

void io_register_write(uint8_t reg, uint8_t value);
void io_write_changed(uint8_t reg);
void io_setup(void);
void io_reset_pc(void);

extern uint8_t io_reg_buffer[0x18];
extern uint8_t io_clockdiv;
