/* 
   NESIZER
   LED interface

   (c) Johan Fjeldtvedt

   Low level LED control. 
*/

#include <avr/io.h>
#include "leds.h"
#include "bus.h"

#define isone(VAL) ((VAL) != 0)

uint8_t leds[5] = {0};
uint8_t row_mirror = 0;

static const uint8_t leds_7seg_values[18] =
{0b11111100, // 0
 0b01100000, // 1
 0b11011010, // 2
 0b11110010, // 3
 0b01100110, // 4
 0b10110110, // 5
 0b10111110, // 6
 0b11100000, // 7
 0b11111110, // 8
 0b11110110, // 9
 0b11101110, // A
 0b00111110, // b
 0b10011100, // C
 0b01111010, // d
 0b10011110, // E
 0b10001110, // F
 0b00000001, // dot
 0b00000010, // minus
};

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
    bus_select(ROW_ADDRESS);
    bus_write(row_mirror);
    
    // Switch to column latch, the row value is latched when this happens
    bus_select(LEDCOL_ADDRESS);
    
    // Activate desired column
    bus_write(current_col);
    
    // Deselect to latch the value
    bus_deselect();

    current_col = (current_col == 0x80) ? 1 : current_col << 1;
}

void leds_7seg_set(uint8_t row, uint8_t val)
{
    leds[row] = leds_7seg_values[val];
}

void leds_7seg_dot_on(uint8_t row) 
{
    leds[row] |= leds_7seg_values[LEDS_7SEG_DOT];
}

void leds_7seg_dot_off(uint8_t row)
{
    leds[row] &= ~leds_7seg_values[LEDS_7SEG_DOT];
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

// For debugging use
void leds_7seg_two_digit_set_hex(uint8_t row1, uint8_t row2, uint8_t value)
{
    leds_7seg_set(row2, value % 16);
    leds_7seg_set(row1, value / 16);
}
