/*
  Copyright 2014-2017 Johan Fjeldtvedt

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



  APU abstraction layer

  Contains abstractions for easily using the 2A03 APU.
*/

#include <stdint.h>
#include "apu.h"
#include "../tools/deltacompress.h"
#include "lfo/lfo.h"
#include "io/2a03.h"

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


/* Square channels */

union sq_vol {
    uint8_t byte;
    struct {
        uint8_t volume_envelope : 4;
        uint8_t constant_volume : 1;
        uint8_t length_cntr_halt : 1;
        uint8_t duty : 2;
    } __attribute__((packed));
};

union sq_sweep {
    uint8_t byte;
    struct {
        uint8_t shift : 3;
        uint8_t negate : 1;
        uint8_t period : 3;
        uint8_t enabled : 1;
    } __attribute__((packed));
};

union sq_lo {
    uint8_t byte;
    uint8_t timer_low;
};

union sq_hi {
    uint8_t byte;
    struct {
        uint8_t timer_high : 3;
        uint8_t length_cntr_load : 5;
    } __attribute__((packed));
};

/* Triangle */

union tri_linear {
    uint8_t byte;
    struct {
        uint8_t linear_counter_load : 7;
        uint8_t length_cntr_disable: 1;
    } __attribute__((packed));
};

union tri_lo {
    uint8_t byte;
    uint8_t timer_low;
};

union tri_hi {
    uint8_t byte;
    struct {
        uint8_t timer_high : 3;
        uint8_t length_cntr_load : 5;
    } __attribute__((packed));
};

/* Noise */

union noise_vol {
    uint8_t byte;
    struct {
        uint8_t volume_envelope : 4;
        uint8_t constant_volume : 1;
        uint8_t length_cntr_halt : 1;
        uint8_t unused : 2;
    } __attribute__((packed));
};

union noise_lo {
    uint8_t byte;
    struct {
        uint8_t period : 4;
        uint8_t unused : 3;
        uint8_t loop : 1;
    } __attribute__((packed));
};

union noise_hi {
    uint8_t byte;
    struct {
        uint8_t unused : 3;
        uint8_t length_cntr_load : 5;
    } __attribute__((packed));
};


struct square sq1, sq2;
struct triangle tri;
struct noise noise;
struct dmc dmc;

inline void register_update(uint8_t reg, uint8_t val)
{
    io_reg_buffer[reg] = val;
}

/* Square channels */

inline void sq_setup(uint8_t n, struct square* sq)
{
    sq->vol = (union sq_vol*)&io_reg_buffer[SQ1_VOL + 4 * n];
    sq->sweep = (union sq_sweep*)&io_reg_buffer[SQ1_SWEEP + 4 * n];
    sq->lo = (union sq_lo*)&io_reg_buffer[SQ1_LO + 4 * n];
    sq->hi = (union sq_hi*)&io_reg_buffer[SQ1_HI + 4 * n];

    sq->vol->length_cntr_halt = 1;
    sq->vol->constant_volume = 1;
    sq->sweep->negate = 1;
    sq->hi->length_cntr_load = 1;
}

inline void sq_update(struct square* sq)
{
    sq->vol->duty = sq->duty;
    sq->vol->volume_envelope = sq->volume;
    sq->lo->timer_low = sq->period & 0xFF;
    sq->hi->timer_high = (sq->period >> 8);
}

void sq1_setup(void)
{
    sq_setup(0, &sq1);
}

void sq2_setup(void)
{
    sq_setup(1, &sq2);
}

void sq1_update(void)
{
    sq_update(&sq1);
}

void sq2_update(void)
{
    sq_update(&sq2);
}


/* Triangle channel */

void tri_setup(void)
{
    tri.linear = (union tri_linear*)&io_reg_buffer[TRI_LINEAR];
    tri.lo = (union tri_lo*)&io_reg_buffer[TRI_LO];
    tri.hi = (union tri_hi*)&io_reg_buffer[TRI_HI];

    tri.linear->length_cntr_disable = 1;
}

void tri_update(void)
{
    tri.lo->timer_low = tri.period & 0xFF;
    tri.hi->length_cntr_load = !tri.silenced ? 1 : 0;
    tri.hi->timer_high = (tri.period >> 8);
    if (!tri.silenced) {
        tri.hi->length_cntr_load = 1;
        tri.linear->length_cntr_disable = 1;
        tri.linear->linear_counter_load = 1;
    }
    else {
        tri.hi->length_cntr_load = 0;
        tri.linear->length_cntr_disable = 0;
        tri.linear->linear_counter_load = 0;
    }
}


/* Noise channel */

void noise_setup(void)
{
    noise.vol = (union noise_vol*)&io_reg_buffer[NOISE_VOL];
    noise.lo = (union noise_lo*)&io_reg_buffer[NOISE_LO];
    noise.hi = (union noise_hi*)&io_reg_buffer[NOISE_HI];

    noise.vol->length_cntr_halt = 1;
    noise.vol->constant_volume = 1;
    noise.hi->length_cntr_load = 1;
}

void noise_update(void)
{
    noise.vol->volume_envelope = noise.volume;
    noise.lo->loop = noise.loop;
    noise.lo->period = noise.period;
}

/* DMC channel */

void dmc_setup(void)
{
    register_update(DMC_FREQ, 0);
    register_update(DMC_RAW, 0);
    register_update(DMC_START, 0);
    register_update(DMC_LEN, 0);
}

void dmc_update_sample_raw(void)
{
    dmc.data = sample_read_byte(&dmc.sample);

    io_register_write(DMC_RAW, dmc.data);

    if (dmc.sample.bytes_done == dmc.sample.size) {
        sample_reset(&dmc.sample);

        if (!dmc.sample_loop)
            dmc.sample_enabled = 0;
    }

}

void dmc_update_sample(void)
{

    if (dmc.sample.type == SAMPLE_TYPE_RAW)
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
    if (dmc.sample_enabled)
        dmc_update_sample();
}

// Setup routine
void apu_setup(void)
{
    sq1_setup();
    sq2_setup();
    tri_setup();
    noise_setup();
    dmc_setup();
    register_update(SND_CHN, 0b11111);
}
