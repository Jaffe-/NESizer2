#include <avr/io.h>
#include "task.h"
#include "apu.h"
#include "2a03_io.h"
#include "leds.h"
#include "lfo.h"
#include "envelope.h"
#include "timing.h"

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
    set_leds(s);
}

void apply_mod()
{
    sq1.period = 400;
    lfo_apply_square(&lfo1, &sq1, 30);
    sq1_update();
}

int main() 
{
    io_setup_2a03();
    leds_setup();
    timer_setup();

    sq1_setup();
    sq1.period = 400;
    sq1.volume = 15;
    sq1.enabled = 1;
    sq1.duty = 2;
    sq1_update();

    tri_setup();
    tri.period = 400;
    tri.enabled = 0;
    tri_update();

    lfo1.period = 1;
    lfo1.waveform = LFO_SINE;

    io_register_write(0x15, 0xFF);
    
    task_add(&update_dmc, 1, 0);
    task_add(&apu_refresh, 10, 1);
    task_add(&update_lfo, 10, 4);
    task_add(&update_env, 400, 20);
    task_add(&apply_mod, 200, 25);
    task_add(&update_led, 8000, 21);
    task_manager();
}
