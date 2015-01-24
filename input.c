#include <avr/io.h>
#include "bus.h"
#include "leds.h"
#include "input.h"

uint8_t input[3] = {0}; 
uint8_t input_analog_value = 0;

void input_refresh() 
/* Rreads one column of switch data each time it is called and auto-increments
   the current row
*/
{
    static uint8_t current_row = 0;
    static uint8_t stage = 0;
    static uint8_t last_data = 0;
    static uint16_t analog_sum = 0;
    
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
	for (uint8_t i = 0; i < 8; i++) {
	    if (((switch_data & (1 << i)) == (last_data & (1 << i)))) 
		*row = (*row & ~(1 << i)) | (switch_data & (1 << i));
	}
        if (++current_row == 3) 
	    current_row = 0;
    }

    stage ^= 1;

    last_data = switch_data;

    // Get analog value and start new conversion. The value is added to a sum of
    // the previous ones. Every sixth function call the sum is divided by 6 to
    // get the actual value. 
    analog_sum += ADCH;    
    ADCSRA |= 0b01000000;
    
    // Every time all rows are completed, it's time to divide the
    // accumulated analog input value by 6 to get a less jittery value.
    if (current_row == 0 && stage == 0) {
	input_analog_value = analog_sum / 6;
	analog_sum = 0;
    }

}

void input_setup()
{
    PORTC &= ~ANALOG_INPUT;
    DDRC &= ~ANALOG_INPUT;

    ADMUX = 0b01100101;     // Select AVCC voltage as reference, ADC5 (PC5) as input

    ADCSRA = 0b10000111;    // Enable ADC, no auto trigger, no interrupt, 128 prescaler
}
