#include <avr/io.h>
#include "leds.h"
#include "bus.h"

/* 
   LED matrix interface
*/

uint8_t leds[32] = {0};
uint8_t row_mirror = 0;

void leds_refresh() 
/* This function is intended to be called by the task handler
   at a given frequency. Each time it is called, a new LED column
   is displayed. 
*/
{
    static uint8_t current_col = 0;

    // Fetch LED status for the rows in the current column
    row_mirror = 0x0F & ~(leds[current_col] 
			  | (leds[current_col + 8] << 1)
			  | (leds[current_col + 8 * 2] << 2)
			  | (leds[current_col + 8 * 3] << 3));
    
    // Address the row latch and put on bus
    bus_set_address(ROW_ADDR);
    bus_set_value(row_mirror);
    
    // Switch to column latch, the row value is latched when this happens
    bus_set_address(LEDCOL_ADDR);
    
    // Activate desired column
    bus_set_value(1 << current_col);

    // Switch to some other address to latch the previous value
    bus_set_address(SWITCHCOL_ADDR);

    if (++current_col == 8) current_col = 0;
}
