#include <avr/io.h>
#include "task.h"
#include "apu.h"
#include "2a03_io.h"
#include "leds.h"
#include "lfo.h"
#include "envelope.h"
#include "timing.h"
#include "modulation.h"
#include <avr/pgmspace.h>
#include "snare_c.c"
//#include "snare_uc.c"

void update_lfo()
{
    lfo_update(&lfo1);
    lfo_update(&lfo2);
}

void update_env()
{
    envelope_update(&env1);
    envelope_update(&env2);
    envelope_update(&env3);
}

void update_dmc()
{
    if (dmc.enabled && dmc.sample_enabled) 
	dmc_update_sample();
}

uint8_t s = 0;

void update_led()
{
    s ^= 1;
    leds_set(s);
    env1.gate = s;
    dmc.sample_enabled = 1;
}

int main() 
{
    io_setup_2a03();
    leds_setup();
    timer_setup();

    dmc_setup();
    dmc.sample = snare_c_raw;
    dmc.sample_length = snare_c_raw_len;
    dmc.enabled = 1;
    dmc.sample_enabled = 1;
    dmc_update();

    sq1_setup();
    sq1.volume = 15;
    sq1.enabled = 0;
    sq1.duty = 2;
    sq1_update();

    tri_setup();
    tri.enabled = 0;
    tri_update();

    bperiods[0] = 400;
    bperiods[2] = 400;

    sq1.period = bperiods[0];

    env1.attack = 30;
    env1.decay = 0;
    env1.sustain = 15;
    env1.release = 30;

    env_mod_select[0] = 0;

    lfo1.period = 2;
    lfo1.waveform = LFO_SINE;

    lfo2.period = 10;
    lfo2.waveform = LFO_RAMP_UP;

    lfo_mod_matrix[0][0] = 20;
    lfo_mod_matrix[0][1] = 0;
    //lfo_mod_matrix[0][1] = 20;

    io_register_write(SND_CHN, SQ1_ENABLE_m | TRI_ENABLE_m);
    
    task_add(&update_dmc, 1, 0);
    task_add(&apu_refresh, 5, 0);
    task_add(&update_lfo, 10, 1);
    task_add(&update_env, 20, 3);
    task_add(&modulation_handler, 20, 4);
    task_add(&update_led, 10000, 6);
    task_manager();
}
