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

#define C4 197
#define D4 176
#define E4 156
#define F4 148
#define G4 131
#define A4 118
#define B4 104
#define C5 98

uint8_t ledrow = 0;

inline uint8_t switch_to_period(uint8_t num)
{
    uint8_t p = 0;
    switch (num) {
    case 0: p = C4; break;
    case 1: p = D4; break;
    case 2: p = E4; break;
    case 3: p = F4; break;
    case 4: p = G4; break;
    case 5: p = A4; break;
    case 6: p = B4; break;
    case 7: p = C5; break;
    }
    return p;
}

void blah()
{
    uint8_t i = 0;
    static uint8_t sq1_last = 0;
    static uint8_t sq2_last = 0;

    if (input[sq1_last] == 0) env1.gate = 0;
    if (input[sq2_last] == 0) env2.gate = 0;

    for (; i < 8; i++) {
	if (input[i]) { 
	    env1.gate = 1;
	    bperiods[0] = switch_to_period(i) << 2;
	    break;
	}
    }
    
    for (i += 1; i < 8; i++) {
	if (input[i]) { 
	    env2.gate = 1;
	    bperiods[1] = switch_to_period(i) << 2;
	    break;
	}
    }

    bperiods[2] = 0;
    for (i = 0; i < 8; i++) {
	if (input[8 + i]) {
	    bperiods[2] = switch_to_period(i) << 2;
	    break;
	}
    }

}

int main() 
{
    io_setup();
    timer_setup();

    sq1_setup();
    sq1.enabled = 1;
    sq1.duty = 2;

    sq2_setup();
    sq2.enabled = 1;
    sq2.duty = 2;

    tri_setup();
    tri.enabled = 1;

    lfo1.period = 15;
    lfo1.waveform = LFO_SINE;

    env1.attack = 50;
    env1.decay = 0;
    env1.sustain = 15;
    env1.release = 50;

    env2.attack = 50;
    env2.decay = 0;
    env2.sustain = 15;
    env2.release = 50;
    
    lfo2.period = 14;
    lfo2.waveform = LFO_SINE;

    lfo_mod_matrix[0][0] = 20;

    lfo_mod_matrix[1][1] = 20;

    env_mod_select[0] = 0;
    env_mod_select[1] = 1;

    leds[0] = 0b11111111;
    leds[2] = 0b11111111;

    leds_7seg_set(2);
    leds_7seg_dot_on();

    task_add(&update_apu, 10, 1);
    task_add(&update_lfo, 10, 2);
    task_add(&update_env, 10, 3);
    task_add(&modulation_handler, 10, 4);
    task_add(&leds_refresh, 20, 5);
    task_add(&input_refresh, 20, 6);
    task_add(&blah, 20, 7);

    task_manager();
    
//    bus_set_address(LEDROW_ADDR);
//    PORTD = 0b11111100;
//    bus_set_address(LEDCOL_ADDR);
//    PORTD = 1;

//    while(1);

    while(1);

}
