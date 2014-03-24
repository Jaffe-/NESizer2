#include "constants.h"
#include <avr/io.h>
#include <util/delay.h>
#include "2a03_io.h"

int main()
{
    //setup_2a03();
    setup_leds();

    uint8_t i = 0;
    uint8_t flag = 0;

    while(1) {
	set_leds(1 << i);
	if (i == 7) 
	    flag = 1;
	if (i == 0)
	    flag = 0;
	if (flag == 1)
	    i--;
	else
	    i++;
	_delay_ms(100);
    }
}
