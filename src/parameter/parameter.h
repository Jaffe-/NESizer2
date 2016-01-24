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



  Programmable parameters

  Contains the list of parameters that can be programmed in a patch. 
*/


#pragma once

#include <stdint.h>

#define NUM_PARAMETERS (ARP_SYNC - SQ1_ENABLED + 1)

struct parameter {
  int8_t* target;
  enum {BOOL, RANGE, INVRANGE} type;
  int8_t min;
  int8_t max;
  int8_t initial_value;
};

enum parameter_id {
    SQ1_ENABLED,
    SQ1_DUTY,
    SQ1_GLIDE,
    SQ1_DETUNE,
    SQ1_LFO1,
    SQ1_LFO2,
    SQ1_LFO3,
    SQ1_PITCHBEND,
    SQ1_COARSE,

    SQ2_ENABLED,
    SQ2_DUTY,
    SQ2_GLIDE,
    SQ2_DETUNE,
    SQ2_LFO1,
    SQ2_LFO2,
    SQ2_LFO3,
    SQ2_PITCHBEND,
    SQ2_COARSE,

    TRI_ENABLED,
    TRI_GLIDE,
    TRI_DETUNE,
    TRI_LFO1,
    TRI_LFO2,
    TRI_LFO3,
    TRI_PITCHBEND,
    TRI_COARSE,

    NOISE_ENABLED,
    NOISE_LOOP,
    NOISE_LFO1,
    NOISE_LFO2,
    NOISE_LFO3,
    NOISE_PITCHBEND,

    DMC_ENABLED,
    DMC_SAMPLE_LOOP,

    ENV1_ATTACK,
    ENV1_DECAY,
    ENV1_SUSTAIN,
    ENV1_RELEASE,

    ENV2_ATTACK,
    ENV2_DECAY,
    ENV2_SUSTAIN,
    ENV2_RELEASE,

    ENV3_ATTACK,
    ENV3_DECAY,
    ENV3_SUSTAIN,
    ENV3_RELEASE,

    LFO1_PERIOD,
    LFO1_WAVEFORM,

    LFO2_PERIOD,
    LFO2_WAVEFORM,

    LFO3_PERIOD,
    LFO3_WAVEFORM,

    ARP_RANGE,
    ARP_MODE,
    ARP_CHANNEL,
    ARP_RATE,
    ARP_SYNC
};

struct parameter parameter_get(uint8_t num);
