#include "constants.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "2a03_io.h"
#include "leds.h"
#include "apu.h"
#include "timing.h"
#include "envelope.h"
#include "lfo.h"
#include "drums.h"

#define C4 197
#define Cs4 186
#define D4 176
#define Ds4 166
#define E4 156
#define F4 148
#define Fs4 139
#define G4 131
#define Gs4 124
#define A4 118
#define As4 110
#define B4 104
#define C5 98

uint16_t basspat[16] = {A4, 0, A4, 0,
			0, 0, 0, E4,
			F4, 0, F4, 0,
			B4, B4, E4, 0};

uint16_t sq1_pat[16] = {A4, A4, A4, A4,
		       B4, B4, C5, C5,
		       C5, C5, C5, C5,
		       C5, C5, C5, C5};

uint16_t sq2_pat[16] = {C4, C4, E4, E4,
		       A4, A4, A4, A4,
		       F4, F4, G4, G4,
		       B4, B4, C4, C4};

uint8_t sq_mask[16] = {1, 1, 1, 0, 
		       1, 0, 1, 1,
		       1, 1, 1, 1,
		       1, 1, 0, 0};

int main()
{
    setup_leds();
    io_setup_2a03();

    noise_setup();
    noise.enabled = 1;
    noise.period = 0;
    noise.loop = 0;
    noise.volume = 0;
    noise.hw_env = 1;
    noise_update();
    
    env1.attack = 0;
    env1.decay = 1;
    env1.sustain = 0;
    env1.release = 0;
    env1.gate_prev = 0;
    
    env2.attack = 2;
    env2.decay = 2;
    env2.sustain = 8;
    env2.release = 3;
    env2.gate_prev = 0;

    sq1_setup();
    sq1.enabled = 1;
    sq1.duty = 1;
    sq1_update();

    sq2_setup();
    sq2.enabled = 1;
    sq2.duty = 2;
    sq2_update();

    tri_setup();
    tri.enabled = 1;

    dmc_setup();
    dmc.enabled = 1;
    dmc.sample_loop = 0;
    dmc_update();

    apu_refresh();

    uint8_t dmc_pat[16] = {DRUM_KICK, EMPTY, DRUM_KICK, EMPTY,
		       DRUM_SNARE, EMPTY, EMPTY, DRUM_KICK,
		       DRUM_KICK, EMPTY, DRUM_KICK, EMPTY,
		       DRUM_SNARE, DRUM_SNARE, EMPTY, EMPTY};
    uint8_t noise_pat[16] = {1, 1, 0, 1,
			     1, 0, 1, 1,
			     1, 1, 0, 1,
			     0, 1, 1, 1};
    dp.dmc_data = dmc_pat;
    dp.noise_data = noise_pat;
    dp.current = 0;
    
    setup_timer();

    while(1) {
    }

}
