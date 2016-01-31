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



  Modulation application

  Applies modulation to sound channels from various sources (LFOs,
  envelopes, portamento, pitchbend, tuning, etc.)
*/


#include <avr/io.h>
#include <avr/pgmspace.h>
#include "periods.h"
#include "modulation/modulation.h"
#include "apu/apu.h"
#include "lfo/lfo.h"
#include "envelope/envelope.h"
#include "portamento/portamento.h"

#define ABS(x) ((x > 0) ? (x) : (-x))

/* Patch programmable parameters */
int8_t mod_lfo_modmatrix[4][3];
int8_t mod_lfo_vol[3];
int8_t mod_detune[3] = {9, 9, 9};   // detune values default to 9 (translates to 0)
int8_t mod_envmod[4] = {9, 9, 9, 7};
int8_t mod_pitchbend[3];
int8_t mod_coarse[3];

/* Input from MIDI  */
uint16_t mod_pitchbend_input[4] = {0x2000, 0x2000, 0x2000, 0x2000};
uint8_t noise_period;

static int16_t dc_temp[3];

static inline int16_t get_pitchbend(uint8_t chn)
{
  return (int16_t)((mod_pitchbend_input[chn] >> 7) - 0x40) * mod_pitchbend[chn];
}

static inline int16_t get_coarse_tune(uint8_t chn)
{
  return (uint16_t)mod_coarse[chn] << 6; 
}

static inline uint16_t apply_dc(uint16_t c, int16_t dc)
{
  int16_t r = c;
  if (r + dc < 0)
    return c;
  else
    return r + dc;
}

static inline void apply_freqmod(uint8_t chn)
/*
  SQ1/2/TRI: Applies calculated frequency modulations by converting them to 
  period compensated period modulations. 

  NOISE: Applies envelope modulation and LFOs directly to period
*/
{
  // Convert frequency delta to a period delta and add to the base period
  uint16_t period = 0;
  if (chn <= CHN_TRI) 
    period = get_period(chn, apply_dc(portamento_cs[chn], dc_temp[chn]));
	
  switch (chn) {
  case CHN_SQ1:
    sq1.period = period;
    break;
  case CHN_SQ2:
    sq2.period = period;
    break;
  case CHN_TRI:
    tri.period = period;
    break;
  case CHN_NOISE:
    noise.period = noise_period;
  }
}

static inline void calc_freqmod(uint8_t chn)
/*
  Calculates frequency change for SQ1, SQ2 and TRI based on 
  detuning, LFOs and envelope modulation. 
*/
{
  int16_t sum = 0;

  for (uint8_t j = 0; j < 3; j++) { 
    if (mod_lfo_modmatrix[chn][j] > 0) 
      sum += mod_lfo_modmatrix[chn][j] * lfo[j].value;
  }
    
  if (chn <= CHN_TRI) {
    int16_t dc = get_coarse_tune(chn);
    
    dc += get_pitchbend(chn);

    dc += sum / 128;
    
    // Add detune frequency delta
    dc += mod_detune[chn];

    // Add envelope modulation, if set
    dc += (int16_t)4 * mod_envmod[chn] * env[chn].value;

    // Store total dc value, which will be applied by apply_freqmod
    dc_temp[chn] = dc;
  }
  else if (chn == CHN_NOISE) {
    noise_period = (env[2].value * (-mod_envmod[chn])) / 8;
  }
}

static inline void apply_volmod(void)
{
  sq1.volume = !mod_lfo_vol[0] ? env[0].value
    : (env[0].value * (8 + ((int16_t)lfo[0].value * mod_lfo_vol[0])/256))/16;
  sq2.volume = !mod_lfo_vol[1] ? env[1].value
    : env[1].value * (8 + ((int16_t)lfo[1].value * mod_lfo_vol[1])/256)/16;
  noise.volume = !mod_lfo_vol[2] ? env[2].value
    : env[2].value * (8 + ((int16_t)lfo[2].value * mod_lfo_vol[2])/256)/16;
}

void mod_calculate(void)
{
  static uint8_t chn = 0;
  calc_freqmod(chn); 
  if (++chn == 4) chn = 0;
}

void mod_apply(void)
{
  static uint8_t chn = 0;
  apply_freqmod(chn); 
  if (++chn == 4) chn = 0;

  apply_volmod();
}
