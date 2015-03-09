#include <avr/io.h>
#include "bus.h"
#include "leds.h"
#include "input.h"

uint8_t input[3] = {0}; 

void input_refresh() 
/* Rreads one column of switch data each time it is called and auto-increments
   the current row
*/
{
    static uint8_t current_row = 0;
    static uint8_t stage = 0;
    static uint8_t last_data = 0;
    
    // Update row latch value
    bus_select(ROW_ADDRESS);
    bus_write(row_mirror | (0x20 << current_row));
   
    nop();
    
    // Latch row value and switch to the switch column input buffer
    bus_select(SWITCHCOL_ADDRESS);
    bus_dir_input();
    uint8_t switch_data = bus_read();
    bus_dir_output();
    bus_deselect();

    if (current_row < 2)
	switch_data = ~switch_data;
    
    if (stage == 1) {	
	// Expand the switch bits into individual bytes in the input array
      uint8_t* row = &input[current_row];

      // Debouncing:
      *row = (switch_data & *row) | (switch_data & last_data) | (last_data & *row);
      
      if (++current_row == 3) 
	    current_row = 0;
    }

    stage ^= 1;

    last_data = switch_data;
    
}
