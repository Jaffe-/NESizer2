#pragma once

#define BTN_SQ1 16
#define BTN_SQ2 17
#define BTN_TRI 18
#define BTN_NOISE 19
#define BTN_DMC 20

#define BTN_SAVE 0
#define BTN_SET 1
#define BTN_PROGRAM 2
#define BTN_PATTERN 3
#define BTN_TRACK 4
#define BTN_SETTINGS 5
#define BTN_UP 6
#define BTN_DOWN 7

#define MODE_PROGRAM 0
#define MODE_PATTERN 1
#define MODE_TRACK 2
#define MODE_SETTINGS 3

#define button_row(BTN) ((BTN) / 8)
#define button_col(BTN) ((BTN) % 8)

// Checks wether a button is being held down
#define button_on_array(ARRAY, BTN) ((ARRAY[button_row(BTN)] & (1 << button_col(BTN))) != 0)
#define button_on(BTN) (button_on_array(input, BTN))

// Was it just pressed?
#define button_pressed(BTN) (button_on_array(input, BTN) && (!button_on_array(prev_input, BTN)))

// Was it just depressed?
#define button_depressed(BTN) (!button_on_array(input, BTN) && button_on_array(prev_input, BTN))

// Gets the boolean value of a particular button
#define button_getbool(BTN) ((input[button_row(BTN)] >> button_col(BTN)) & 1)

void ui_handler();
void ui_leds_handler();

// The current mode (PROGRAM, PATTERN, TRACK or SETTINGS)
uint8_t mode;

// Previous inputs
uint8_t prev_input[3];

/* Button leds format: 

   0 - off
   0xFF - led on
   any other value: blink
*/
uint8_t button_leds[24];

void updown(uint8_t* value, uint8_t min, uint8_t max);
