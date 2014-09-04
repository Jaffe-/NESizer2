#include <avr/io.h>
#include <avr/pgmspace.h>
#include "task.h"
#include "apu.h"
#include "2a03_io.h"
#include "leds.h"
#include "lfo.h"
#include "envelope.h"
#include "timing.h"
#include "modulation.h"
#include "input.h"
#include "user_interface.h"
#include "memory.h"
#include "bus.h"
#include "patch.h"
#include "sequencer.h"
#include "sample.h"
#include "midi.h"
#include "midi_interpreter.h"

void update_lfo()
{
    lfo_update(&lfo1);
    lfo_update(&lfo2);
    lfo_update(&lfo3);
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

void update_apu()
{
    static uint8_t chn = CHN_SQ1;

    apu_update_channel(chn);
    apu_refresh_channel(chn);
    if (++chn == 4) chn = 0;
}

int main() 
{
    DDRB = ADDR_m;
    bus_set_address(CPU_ADDR);

    io_setup();
    midi_setup();
    input_setup();
    memory_setup();
    timer_setup();

    sq1_setup();
    sq2_setup();
    tri_setup();
    noise_setup();
    dmc_setup();

    patch_load(0);

    task_add(&update_dmc, 1, 0);
    task_add(&midi_handler, 5, 0);
    task_add(&update_apu, 10, 1);
    task_add(&update_lfo, 10, 2);
    task_add(&update_env, 10, 3);
    task_add(&modulation_handler, 10, 4);
    task_add(&midi_interpreter_handler, 10, 5);
    task_add(&leds_refresh, 20, 5); // 20
    task_add(&input_refresh, 80, 6);
    task_add(&sequence_handler, 80, 7);
    task_add(&ui_handler, 80, 8);
    task_add(&ui_leds_handler, 80, 9);

    task_manager();

}
