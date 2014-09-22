#include <avr/io.h>
#include "memory.h"
#include "patch.h"
#include "apu.h"
#include "envelope.h"
#include "lfo.h"
#include "modulation.h"

#define PATCH_OFFSET 0

#define SQ1_SIZE 2
#define SQ2_SIZE 2
#define TRI_SIZE 1
#define NOISE_SIZE 2
#define DMC_SIZE 3
#define ENV_SIZE 4
#define LFO_SIZE 6

#define SQ1_OFFSET 0
#define SQ2_OFFSET (SQ1_OFFSET + SQ1_SIZE)
#define TRI_OFFSET (SQ2_OFFSET + SQ2_SIZE)
#define NOISE_OFFSET (TRI_OFFSET + TRI_SIZE)
#define DMC_OFFSET (NOISE_OFFSET + NOISE_SIZE)
#define ENV1_OFFSET (DMC_OFFSET + DMC_SIZE)
#define ENV2_OFFSET (ENV1_OFFSET + ENV_SIZE)
#define ENV3_OFFSET (ENV2_OFFSET + ENV_SIZE)
#define LFO1_OFFSET (ENV3_OFFSET + ENV_SIZE)
#define LFO2_OFFSET (LFO1_OFFSET + LFO_SIZE)
#define LFO3_OFFSET (LFO2_OFFSET + LFO_SIZE)

#define PATCH_SIZE (SQ1_SIZE + SQ2_SIZE + TRI_SIZE + NOISE_SIZE + DMC_SIZE + 3 * ENV_SIZE + 3 * LFO_SIZE)

const uint16_t PATCH_MEMORY_END = PATCH_OFFSET + (PATCH_MAX + 1 - PATCH_MIN) * PATCH_SIZE;

void patch_clean()
{
    for (uint16_t i = 0; i < PATCH_MAX * PATCH_SIZE; i++) 
	memory_write(i, 0);
}

void patch_save(uint8_t num) 
{
    // Patches are stored sequentually from RAM address 0, so the base address of patch
    // number num is simply num * PATH_SIZE:
    uint16_t base_address = PATCH_SIZE * num;
    
    memory_write(base_address + SQ1_OFFSET, sq1.enabled);
    memory_write(base_address + SQ1_OFFSET + 1, sq1.duty);

    memory_write(base_address + SQ2_OFFSET, sq2.enabled);
    memory_write(base_address + SQ2_OFFSET + 1, sq2.duty);

    memory_write(base_address + TRI_OFFSET, tri.enabled);

    memory_write(base_address + NOISE_OFFSET, noise.enabled);
    memory_write(base_address + NOISE_OFFSET + 1, noise.loop);

    memory_write(base_address + DMC_OFFSET, dmc.enabled);
    memory_write(base_address + DMC_OFFSET + 1, dmc.sample_number);
    memory_write(base_address + DMC_OFFSET + 2, dmc.sample_loop);

    memory_write(base_address + ENV1_OFFSET, env1.attack);
    memory_write(base_address + ENV1_OFFSET + 1, env1.decay);
    memory_write(base_address + ENV1_OFFSET + 2, env1.sustain);
    memory_write(base_address + ENV1_OFFSET + 3, env1.release);

    memory_write(base_address + ENV2_OFFSET, env2.attack);
    memory_write(base_address + ENV2_OFFSET + 1, env2.decay);
    memory_write(base_address + ENV2_OFFSET + 2, env2.sustain);
    memory_write(base_address + ENV2_OFFSET + 3, env2.release);

    memory_write(base_address + ENV3_OFFSET, env3.attack);
    memory_write(base_address + ENV3_OFFSET + 1, env3.decay);
    memory_write(base_address + ENV3_OFFSET + 2, env3.sustain);
    memory_write(base_address + ENV3_OFFSET + 3, env3.release);

    memory_write(base_address + LFO1_OFFSET, lfo1.period & 0xFF);
    memory_write(base_address + LFO1_OFFSET + 1, lfo1.period >> 8);
    memory_write(base_address + LFO1_OFFSET + 2, lfo1.waveform);
    memory_write(base_address + LFO1_OFFSET + 3, mod_lfo_modmatrix[0][0]);
    memory_write(base_address + LFO1_OFFSET + 4, mod_lfo_modmatrix[0][1]);
    memory_write(base_address + LFO1_OFFSET + 5, mod_lfo_modmatrix[0][2]);

    memory_write(base_address + LFO2_OFFSET, lfo2.period & 0xFF);
    memory_write(base_address + LFO2_OFFSET + 1, lfo2.period >> 8);
    memory_write(base_address + LFO2_OFFSET + 2, lfo2.waveform);
    memory_write(base_address + LFO2_OFFSET + 3, mod_lfo_modmatrix[1][0]);
    memory_write(base_address + LFO2_OFFSET + 4, mod_lfo_modmatrix[1][1]);
    memory_write(base_address + LFO2_OFFSET + 5, mod_lfo_modmatrix[1][2]);

    memory_write(base_address + LFO3_OFFSET, lfo3.period & 0xFF);
    memory_write(base_address + LFO3_OFFSET + 1, lfo3.period >> 8);
    memory_write(base_address + LFO3_OFFSET + 2, lfo3.waveform);
    memory_write(base_address + LFO3_OFFSET + 3, mod_lfo_modmatrix[2][0]);
    memory_write(base_address + LFO3_OFFSET + 4, mod_lfo_modmatrix[2][1]);
    memory_write(base_address + LFO3_OFFSET + 5, mod_lfo_modmatrix[2][2]);
}

void patch_load(uint8_t num)
{
    uint16_t base_address = PATCH_SIZE * num;
    
    sq1.enabled = memory_read(base_address + SQ1_OFFSET);
    sq1.duty = memory_read(base_address + SQ1_OFFSET + 1);

    sq2.enabled = memory_read(base_address + SQ2_OFFSET);
    sq2.duty = memory_read(base_address + SQ2_OFFSET + 1);

    tri.enabled = memory_read(base_address + TRI_OFFSET);

    noise.enabled = memory_read(base_address + NOISE_OFFSET);
    noise.loop = memory_read(base_address + NOISE_OFFSET + 1);

    dmc.enabled = memory_read(base_address + DMC_OFFSET);
    dmc.sample_number = memory_read(base_address + DMC_OFFSET + 1);
    dmc.sample_loop = memory_read(base_address + DMC_OFFSET + 2);

    env1.attack = memory_read(base_address + ENV1_OFFSET);
    env1.decay = memory_read(base_address + ENV1_OFFSET + 1);
    env1.sustain = memory_read(base_address + ENV1_OFFSET + 2);
    env1.release = memory_read(base_address + ENV1_OFFSET + 3);

    env2.attack = memory_read(base_address + ENV2_OFFSET);
    env2.decay = memory_read(base_address + ENV2_OFFSET + 1);
    env2.sustain = memory_read(base_address + ENV2_OFFSET + 2);
    env2.release = memory_read(base_address + ENV2_OFFSET + 3);

    env3.attack = memory_read(base_address + ENV3_OFFSET);
    env3.decay = memory_read(base_address + ENV3_OFFSET + 1);
    env3.sustain = memory_read(base_address + ENV3_OFFSET + 2);
    env3.release = memory_read(base_address + ENV3_OFFSET + 3);

    lfo1.period = memory_read(base_address + LFO1_OFFSET);
    lfo1.period |= memory_read(base_address + LFO1_OFFSET + 1) << 8;
    lfo1.waveform = memory_read(base_address + LFO1_OFFSET + 2);
    mod_lfo_modmatrix[0][0] = memory_read(base_address + LFO1_OFFSET + 3);
    mod_lfo_modmatrix[0][1] = memory_read(base_address + LFO1_OFFSET + 4);
    mod_lfo_modmatrix[0][2] = memory_read(base_address + LFO1_OFFSET + 5);

    lfo2.period = memory_read(base_address + LFO2_OFFSET);
    lfo2.period |= memory_read(base_address + LFO2_OFFSET + 1) << 8;
    lfo2.waveform = memory_read(base_address + LFO2_OFFSET + 2);
    mod_lfo_modmatrix[1][0] = memory_read(base_address + LFO2_OFFSET + 3);
    mod_lfo_modmatrix[1][1] = memory_read(base_address + LFO2_OFFSET + 4);
    mod_lfo_modmatrix[1][2] = memory_read(base_address + LFO2_OFFSET + 5);

    lfo3.period = memory_read(base_address + LFO3_OFFSET);
    lfo3.period |= memory_read(base_address + LFO3_OFFSET + 1) << 8;
    lfo3.waveform = memory_read(base_address + LFO3_OFFSET + 2);
    mod_lfo_modmatrix[2][0] = memory_read(base_address + LFO3_OFFSET + 3);
    mod_lfo_modmatrix[2][1] = memory_read(base_address + LFO3_OFFSET + 4);
    mod_lfo_modmatrix[2][2] = memory_read(base_address + LFO3_OFFSET + 5);
}
