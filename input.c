#include <avr/io.h>
#include "bus.h"
#include "leds.h"

uint8_t input[32] = {0}; 

// Private for debouncing
uint8_t last_input[4] = {0};

void input_refresh() 
/* Rreads one column of switch data each time it is called and auto-increments
   the current row
*/
{
    static uint8_t current_row = 0;
    static uint8_t stage = 0;
    static uint8_t last_data = 0;

    // Update row latch value
    bus_set_value(row_mirror | (0x10 << current_row));
    bus_set_address(ROW_ADDR);
   
    nop();
    
    // Latch row value and switch to the switch column input buffer
    bus_set_address(SWITCHCOL_ADDR);
    bus_set_input();
    uint8_t switch_data = bus_read_value();
    bus_set_output();
    bus_set_address(NO_ADDR);

    if (stage == 1) {	
	// Expand the switch bits into individual bytes in the input array
	uint8_t* row = &input[current_row * 8];
	for (uint8_t i = 0; i < 8; i++) {
	    if (((switch_data & (1 << i)) == (last_data & (1 << i)))) 
		row[i] = (switch_data >> i) & 1;
	}
        if (++current_row == 4) current_row = 0;
    }

    stage ^= 1;

    last_data = switch_data;
}
