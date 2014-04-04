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

    sq1_setup();
    sq1.enabled = 1;
    sq1.duty = 2;
    sq1.period = 400;
    sq1.volume = 5;
    sq1_update();

    sq2_setup();
    sq2.enabled = 1;
    sq2.duty = 2;
    sq2.period = 402;
    sq2.volume = 5;
    sq2_update();

    tri_setup();
    tri.enabled = 1;
    tri_update();
    apu_refresh();

    set_leds(reg_buffer[SQ1_LO]);

    uint8_t i = 1;
    uint8_t f = 0;
    uint16_t j = 8;
    uint8_t f2 = 0;

    while(1) {
	if (j == 2047) f2 = 1;
	if (j == 8) f2 = 0;

	if (f2) 
	    j--;
	else
	    j++;

	i = j >> 7;

	sq1.volume = i;
	sq2.volume = i;
	tri.period = j;
	sq1_update();
	sq2_update();
	tri_update();
	apu_refresh();
	set_leds(1 << (j >> 8));
	_delay_us(10);
    
    };

}
