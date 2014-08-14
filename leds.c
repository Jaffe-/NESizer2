#include <avr/io.h>
#include "leds.h"
#include "bus.h"

#define isone(VAL) ((VAL) != 0)

/* 
   LED matrix interface
*/

uint8_t leds[5] = {0};
uint8_t row_mirror = 0;

const uint8_t leds_7seg_values[10] = {0b01110111,
				      0b00010100,
				      0b10110011,
				      0b10110110,
				      0b11010100,
				      0b11100110,
				      0b11100111,
				      0b00110100,
				      0b11110111,
				      0b11110100};

void leds_refresh() 
/* This function is intended to be called by the task handler
   at a given frequency. Each time it is called, a new LED column
   is displayed. 
*/
{
    static uint8_t current_col = 1;

    // Fetch LED status for the rows in the current column
    row_mirror = 0x1F & ~(isone(leds[0] & current_col)  
			  | (isone(leds[1] & current_col) << 1)
			  | (isone(leds[2] & current_col) << 2)
	                  | (isone(leds[3] & current_col) << 3)
	                  | (isone(leds[4] & current_col) << 4));
    
    // Address the row latch and put on bus
    bus_set_address(ROW_ADDR);
    bus_set_value(row_mirror);
    
    // Switch to column latch, the row value is latched when this happens
    bus_set_address(LEDCOL_ADDR);
    
    // Activate desired column
    bus_set_value(current_col);

    // Switch to some other address to latch the previous value
    bus_set_address(NO_ADDR);

    current_col = (current_col == 0x80) ? 1 : current_col << 1;
}

void leds_7seg_set(uint8_t row, uint8_t val)
{
    leds[row] = leds_7seg_values[val];
}

void leds_7seg_dot_on(uint8_t row) 
{
    leds[row] |= 0b1000;
}

void leds_7seg_dot_off(uint8_t row)
{
    leds[row] &= ~(0b1000);
}

void leds_7seg_clear(uint8_t row)
{
    leds[row] = 0;
}

void leds_7seg_two_digit_set(uint8_t row1, uint8_t row2, uint8_t value)
{
    leds_7seg_set(row2, value % 10);
    leds_7seg_set(row1, value / 10);
}
