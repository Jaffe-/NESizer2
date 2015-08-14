/*
  Copyright 2014-2015 Johan Fjeldtvedt 

  This file is part of NESIZER.

  NESIZER is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  NESIZER is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NESIZER.  If not, see <http://www.gnu.org/licenses/>.



  input.c - Button io/input.handling

  Reads button states
*/


#include <avr/io.h>
#include "io/bus.h"
#include "io/leds.h"
#include "io/input.h"

uint8_t input[3]; 

void input_refresh(void) 
/* Reads one column of switch data each time it is called and auto-increments
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
