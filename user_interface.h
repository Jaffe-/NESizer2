#pragma once

#define BTN_SQ1 16
#define BTN_SQ2 17
#define BTN_TRI 18
#define BTN_NOISE 19
#define BTN_DMC 20
#define BTN_LFO1 21
#define BTN_LFO2 22
#define BTN_LFO3 23

#define BTN_DUTY 8
#define BTN_LOOP 9
#define BTN_WAVE 10
#define BTN_SAMPLE 11
#define BTN_A 12
#define BTN_D 13
#define BTN_S 14
#define BTN_R 15

#define BTN_SAVE 0
#define BTN_SET 1
#define BTN_PRG 2
#define BTN_SEQ 3
#define BTN_SYS 4
#define BTN_MIDI 5
#define BTN_UP 6
#define BTN_DOWN 7

#define MODE_PROGRAMMER 0
#define MODE_SEQUENCER 1
#define MODE_SETTINGS 2
#define MODE_MIDI 3

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

// Previous inputs
uint8_t prev_input[3];

/* Button leds format: 

   0 - off
   0xFF - led on
   any other value: blink
*/
uint8_t button_leds[24];
