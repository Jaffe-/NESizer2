#include "constants.h"
#include <avr/io.h>
#include <util/delay.h>
#include "2a03_io.h"
#include "leds.h"
#include "apu.h"

int main()
{
    setup_leds();
    setup_2a03();

    register_write(SND_CHN, 0b0111);
    register_write(SQ1_VOL, 0b10111111);
    register_write(SQ1_LO, 140);
    register_write(SQ1_HI, 0x11);

    register_write(SQ2_VOL, 0b10111111);
    register_write(SQ2_LO, 139);
    register_write(SQ2_HI, 0x11);

    register_write(TRI_LINEAR, 0b10000001);
    register_write(TRI_LO, 0);
    register_write(TRI_HI, 2);

    register_write(NOISE_VOL, 0b00100001);
    register_write(NOISE_LO, 0b00000001);
    register_write(NOISE_HI, 0);

    uint8_t v = read_status();
    
    set_leds(v);

    while(1) {
    };

}
