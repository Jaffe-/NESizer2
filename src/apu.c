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



  apu.c - APU abstraction layer

  Contains abstractions for easily using the 2A03 APU.
*/


#include "apu.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "2a03_io.h"
#include "../tools/deltacompress.h"
#include "lfo.h"

/* APU channel bit masks */

#define SQ1_bm 1
#define SQ2_bm 0b10
#define TRI_bm 0b100
#define NOISE_bm 0b1000
#define DMC_bm 0b10000


/* The APU registers (discarding the 0x40 upper byte) */

#define SQ1_VOL 0x00
#define SQ1_SWEEP  0x01
#define SQ1_LO 0x02
#define SQ1_HI 0x03

#define SQ2_VOL 0x04
#define SQ2_SWEEP 0x05
#define SQ2_LO 0x06
#define SQ2_HI 0x07

#define TRI_LINEAR 0x08
#define TRI_LO 0x0A
#define TRI_HI 0x0B

#define NOISE_VOL 0x0C
#define NOISE_LO 0x0E
#define NOISE_HI 0x0F

#define DMC_FREQ 0x10
#define DMC_RAW   0x11
#define DMC_START 0x12
#define DMC_LEN 0x13

#define SND_CHN 0x15


/* Common bit masks and positions */

#define VOLUME_m 0b00001111
#define PERIOD_HI_m 0b00000111

#define VOLUME_p 0
#define PERIOD_HI_p 0

#define LENGTH_CNTR_LOAD_p 3


/* Square channels */

// flags
#define SQ_LENGTH_CNTR_DISABLE 0b00100000
#define SQ_CONSTANT_VOLUME 0b00010000

// bit positions
#define SQ_DUTY_p 6
#define SQ_VOLUME_p 0
#define SQ_LENGTH_CNTR_LOAD_p 3

// masks
#define SQ_DUTY_m 0b11000000


/* Triangle channel */

// flags
#define TRI_LENGTH_CNTR_DISABLE 0b10000000

// bit positions
#define TRI_LINEAR_RELOAD_bp 0
#define TRI_LENGTH_CNTR_LOAD_bp 3


/* Noise channel */

// flags
#define NOISE_LENGTH_CNTR_DISABLE 0b00100000
#define NOISE_CONSTANT_VOLUME 0b00010000

// bit positions
#define NOISE_LENGTH_CNTR_LOAD_bp 3
#define NOISE_LOOP_p 7
#define NOISE_PERIOD_p 0
#define NOISE_HW_ENV_p 4

// masks
#define NOISE_LOOP_m 0b10000000
#define NOISE_PERIOD_m 0b00001111
#define NOISE_HW_ENV_m 0b00010000

/* DMC channel */

// flags
#define DMC_IRQ_ENABLE 0b10000000
#define DMC_LOOP_SAMPLE 0b01000000


/* Control register */

#define DMC_ENABLE_m 0b00010000
#define NOISE_ENABLE_m 0b00001000
#define TRI_ENABLE_m 0b00000100
#define SQ2_ENABLE_m 0b00000010
#define SQ1_ENABLE_m 0b00000001

#define DMC_ENABLE_p 4
#define NOISE_ENABLE_p 3
#define TRI_ENABLE_p 2
#define SQ2_ENABLE_p 1
#define SQ1_ENABLE_p 0

/* 
   APU abstraction layer 

   Contains functions for putting channel data in registers. 

*/

struct square sq1, sq2;
struct triangle tri;
struct noise noise;
struct dmc dmc;

inline void register_update(uint8_t reg, uint8_t val)
{
  io_reg_buffer[reg] = val;
}

/* Square channels */

inline void sq_setup(uint8_t n)
{
  register_update(SQ1_VOL + n * 4, SQ_LENGTH_CNTR_DISABLE | SQ_CONSTANT_VOLUME);
  register_update(SQ1_SWEEP + n * 4, 0x08);
  register_update(SQ1_HI + n * 4, 1 << LENGTH_CNTR_LOAD_p);
}

inline void sq_update(uint8_t n, struct square* sq)
{
  register_update(SQ1_VOL + n * 4, (io_reg_buffer[SQ1_VOL + n * 4] & ~(SQ_DUTY_m | VOLUME_m)) 
		  | sq->volume << VOLUME_p
		  | sq->duty << SQ_DUTY_p);

  register_update(SQ1_LO + n * 4, sq->period & 0xFF);

  // Need to check if sq.enabled is true to decide the value of the length counter. 
  // It is set to zero whenever the corresponding SND_CHN bit is cleared, and this needs
  // to be reflected in the register mirror.
  register_update(SQ1_HI + n * 4, ((sq->enabled) ? 0b1000 : 0) 
		  | (((sq->period >> 8) & 0x07) << PERIOD_HI_p));

}

void sq1_setup(void)
{
  sq_setup(0);
}

void sq2_setup(void)
{
  sq_setup(1);
}

void sq1_update(void)
{
  register_update(SND_CHN, (io_reg_buffer[SND_CHN] & ~SQ1_ENABLE_m) 
		  | sq1.enabled << SQ1_ENABLE_p);

  sq_update(0, &sq1);
}

void sq2_update(void)
{
  register_update(SND_CHN, (io_reg_buffer[SND_CHN] & ~SQ2_ENABLE_m) 
		  | sq2.enabled << SQ2_ENABLE_p);

  sq_update(1, &sq2);
}


/* Triangle channel */

void tri_setup(void)
{
  register_update(TRI_LINEAR, TRI_LENGTH_CNTR_DISABLE | 1);
//    register_update(TRI_HI, 0b0000);
}

void tri_update(void)
{
  register_update(SND_CHN, (io_reg_buffer[SND_CHN] & ~TRI_ENABLE_m) 
		  | tri.enabled << TRI_ENABLE_p);

  register_update(TRI_LO, tri.period & 0xFF);

  register_update(TRI_HI, (!tri.silenced ? 0b1000 : 0) 
		  | ((tri.period >> 8) & 0x07) << PERIOD_HI_p);

  register_update(TRI_LINEAR, tri.silenced ? 0 : (TRI_LENGTH_CNTR_DISABLE | 1)); 
  
}


/* Noise channel */

void noise_setup(void){

  register_update(NOISE_VOL, NOISE_LENGTH_CNTR_DISABLE | NOISE_CONSTANT_VOLUME);
  register_update(NOISE_HI, 0b1000);
}

void noise_update(void)
{
  register_update(SND_CHN, (io_reg_buffer[SND_CHN] & ~NOISE_ENABLE_m) 
		  | noise.enabled << NOISE_ENABLE_p);

  register_update(NOISE_VOL, (io_reg_buffer[NOISE_VOL] & ~VOLUME_m)
		  | noise.volume << VOLUME_p);

  register_update(NOISE_LO, (io_reg_buffer[NOISE_VOL] & ~(NOISE_LOOP_m | NOISE_PERIOD_m))
		  | noise.loop << NOISE_LOOP_p 
		  | noise.period << NOISE_PERIOD_p);

  register_update(NOISE_HI, (noise.enabled) ? 0b1000 : 0);
}

/* DMC channel */

void dmc_setup(void)
{
  register_update(DMC_FREQ, 0);
  register_update(DMC_RAW, 0);
  register_update(DMC_START, 0);
  register_update(DMC_LEN, 0);
}

void dmc_update(void)
{
  register_update(SND_CHN, (io_reg_buffer[SND_CHN] & ~DMC_ENABLE_m) 
		  | dmc.enabled << DMC_ENABLE_p);
}

void dmc_update_sample_raw(void)
{  
  dmc.data = sample_read_byte();
   
  io_register_write(DMC_RAW, dmc.data);
  
  if (sample.bytes_done == sample.size) {
    sample_reset();

    if (!dmc.sample_loop) 
      dmc.sample_enabled = 0;
  }
	
}

/*
  void dmc_update_sample_dpcm() 
  {
  static uint8_t data;
  static int8_t accumulator;
  static uint8_t flag;

  if (dmc.sample.bytes_done == 0) {
  accumulator = sample_read_byte(&dmc.sample); 
  flag = 1;
  }
  else if (!flag) {
  data = sample_read_byte(&dmc.sample);
  accumulator += delta_table[data & 0x0F];
  }
  else {
  accumulator += delta_table[(data >> 4) & 0x0F];
  }
  flag ^= 1;
    
  dmc.data = accumulator >> 1;
    
  register_update(DMC_RAW, dmc.data);
  io_register_write(DMC_RAW, dmc.data);
    
  if (dmc.sample.bytes_done == dmc.sample.size) {
  sample_reset(&dmc.sample);
  if (!dmc.sample_loop) {
  dmc.sample_enabled = 0;
  io_register_write(DMC_RAW, 0);
  }
  }
  }
*/

void dmc_update_sample(void)
{

  if (sample.type == SAMPLE_TYPE_RAW)
    dmc_update_sample_raw();
//    else
//	dmc_update_sample_dpcm();

}

void apu_refresh_channel(uint8_t ch_number)
{  
  io_write_changed(SND_CHN);

  switch (ch_number) {
  case CHN_SQ1:
    io_write_changed(SQ1_VOL);
    io_write_changed(SQ1_LO);
    io_write_changed(SQ1_HI);
    break;

  case CHN_SQ2:
    io_write_changed(SQ2_VOL);
    io_write_changed(SQ2_LO);
    io_write_changed(SQ2_HI);
    break;

  case CHN_TRI:
    io_write_changed(TRI_LINEAR);
    io_write_changed(TRI_LO);
    io_write_changed(TRI_HI);
    break;

  case CHN_NOISE:
    io_write_changed(NOISE_VOL);
    io_write_changed(NOISE_LO);
    io_write_changed(NOISE_HI);
    break;
  }
   
}

void apu_refresh_all(void)
{
  for (uint8_t i = CHN_SQ1; i <= CHN_DMC; i++) 
    apu_refresh_channel(i);
}

inline void apu_update_channel(uint8_t chn)
{
  switch (chn) {
  case CHN_SQ1:
    sq1_update(); break;
  case CHN_SQ2:
    sq2_update(); break;
  case CHN_TRI:
    tri_update(); break;
  case CHN_NOISE:
    noise_update(); break;
  }
}

// Task handler for updating APU channels
void apu_update_handler(void)
{
  static uint8_t chn = CHN_SQ1;

  apu_update_channel(chn);
  apu_refresh_channel(chn);
  // Keep 6502's PC in check 
  io_reset_pc();

  if (++chn == 4) 
    chn = 0;
  
}

// Update handler for the DMC specifically
void apu_dmc_update_handler(void)
{
  if (dmc.enabled && dmc.sample_enabled) 
    dmc_update_sample();
}

// Setup routine
void apu_setup()
{
  sq1_setup();
  sq2_setup();
  tri_setup();
  noise_setup();
  dmc_setup();
}
