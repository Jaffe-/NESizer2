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



  apu.h - APU abstraction layer

  Contains abstractions for easily using the 2A03 APU.
*/


#pragma once

#include "sample.h"

/* The channels */

#define CHN_SQ1 0
#define CHN_SQ2 1
#define CHN_TRI 2
#define CHN_NOISE 3
#define CHN_DMC 4

struct square {
  int8_t enabled;          // BOOL
  int8_t duty;             // 2-bit integer

  // Used internally:
  uint8_t volume;
  uint16_t period;
};

struct triangle { 
  int8_t enabled;          // BOOL 
  int8_t silenced;          // BOOL

  // Used internally:
  uint16_t period;
};

struct noise {
  int8_t enabled;
  int8_t loop;             // BOOL: Loop mode (pitched noise) 

  // Used internally:
  uint8_t volume : 4;
  uint8_t period : 4;
};

struct dmc {
  int8_t enabled;
  int8_t sample_loop;       // BOOL: Wether or not sample is automatically looped
  uint8_t sample_number;

  // Used internally:
  uint8_t sample_enabled : 1;
  uint8_t data : 7;

  struct sample sample;
};

extern struct square sq1, sq2;
extern struct triangle tri;
extern struct noise noise;
extern struct dmc dmc;

/* Functions */

void sq1_setup(void);
void sq1_update(void);
void sq2_setup(void);
void sq2_update(void);
void tri_setup(void);
void tri_update(void);
void noise_setup(void);
void noise_update(void);
void dmc_setup(void);
void dmc_update(void);
void dmc_update_sample(void);
void apu_refresh_channel(uint8_t);
void apu_refresh_all(void);
void apu_update_handler(void);
void apu_dmc_update_handler(void);
void apu_setup(void);
