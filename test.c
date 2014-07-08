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
#include "kick.c"
#include "snare.c"
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

void update_led()
{
    static uint8_t s = 0;
    s ^= 1;
    leds_set(s);
    env1.gate = s;
    if (s) {
	dmc.sample = kick_c_raw;
	dmc.sample_length = kick_c_raw_len;
	dmc.current = 0;
    }
    else {
	dmc.sample = snare_c_raw;
	dmc.sample_length = snare_c_raw_len;
	dmc.current = 0;
    }
    dmc.sample_enabled = 1;
}

int main() 
{
    io_setup();
    leds_setup();

    sq1_setup();

    sq1.enabled = 1;
    sq1.period = 400;
    sq1.volume = 15;
    sq1.duty = 2;

    sq1_update();

    leds_set(io_reg_buffer[SND_CHN]);

    apu_refresh_channel(CHN_SQ1);
    
    while(1);

}
