#include "constants.h"
#include <avr/io.h>
#include <util/delay.h>
#include "2a03_io.h"
#include "leds.h"
#include "apu.h"
#include "timing.h"
#include "envelope.h"
#include "lfo.h"

int main()
{
    setup_leds();
    setup_2a03();
    setup_timer();

    sq1_setup();
    sq1.enabled = 1;
    sq1.volume = 15;
    sq1.base_period = 400;
    sq1.period = 400;
    sq1.duty = 2;

    sq2_setup();
    sq2.enabled = 1;
    sq2.volume = 15;
    sq2.base_period = 400;
    sq2.period = 400;
    sq2.duty = 2;

    sq1_update();
    sq2_update();

    lfo1.period = 40;
    lfo1.intensity = 60;
    lfo1.waveform = LFO_SINE;

    lfo2.period = 100;
    lfo2.intensity = 50;
    lfo2.waveform = LFO_RAMP_UP;

    while(1) {
    }
}
