#include <avr/io.h>
#include <avr/delay.h>
#include <util/atomic.h>
#include "task.h"
#include "apu.h"
#include "2a03_io.h"
#include "leds.h"
#include "lfo.h"
#include "envelope.h"
#include "timing.h"
#include "modulation.h"
#include "input.h"
#include "drummachine.h"
#include <avr/pgmspace.h>
//#include "kick.c"
//#include "snare.c"
#include "bus.h"
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
	dmc_update_sample_dpcm();
}

void update_apu()
{
    static uint8_t chn = CHN_SQ1;

    apu_refresh_channel(chn);
    if (++chn == 5) chn = 0;
}

int main() 
{
    io_setup();
    timer_setup();

    sq1_setup();
    sq1.enabled = 1;
    sq1.volume = 0;
    sq1.duty = 2;

    sq2_setup();
    sq2.enabled = 1;
    sq2.volume = 0;
    sq2.duty = 1;

    tri_setup();
    tri.enabled = 1;

    noise_setup();
    noise.enabled = 1;
    noise.period = 0;
    noise.loop = 0;
    noise.volume = 0;

    dmc_setup();
    dmc.enabled = 1;
    dmc.sample_enabled = 0;

    env1.attack = 0;
    env1.decay = 2;
    env1.sustain = 10;
    env1.release = 25;
    env1.gate = 0;

    env2.attack = 40;
    env2.decay = 0;
    env2.sustain = 15;
    env2.release = 40;
    env2.gate = 0;

    env3.attack = 0;
    env3.decay = 2;
    env3.sustain = 5;
    env3.release = 10;
  
    env_mod_select[1] = 1;
    env_mod_select[2] = 2;

    lfo1.period = 3;
    lfo1.waveform = LFO_SINE;
    
    lfo_mod_matrix[0][0] = 20;

    leds_7seg_set(8);

    task_add(&update_dmc, 1, 0);
    task_add(&update_apu, 10, 1);
    task_add(&update_lfo, 10, 2);
    task_add(&update_env, 10, 3);
    task_add(&modulation_handler, 10, 4);
    task_add(&leds_refresh, 20, 5);
    task_add(&input_refresh, 80, 6);
    task_add(&drum_task, 80, 7);
    task_add(&drum_update_leds, 80, 9);

    task_manager();
    
//    bus_set_address(LEDROW_ADDR);
//    PORTD = 0b11111100;
//    bus_set_address(LEDCOL_ADDR);
//    PORTD = 1;

//    while(1);

    while(1);

}
