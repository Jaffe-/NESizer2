/*
  NESIZER
  General user interface routines
  
  (c) Johan Fjeldtvedt

  Contains the UI handler which checks button presses
  and transfers control to the corresponding mode handlers,
  and the LED refresh handler
 */

#pragma once

#include "parameter.h"
#include "input.h"
#include "leds.h"

#define BTN_SQ1 0 // 16
#define BTN_SQ2 1 // 17
#define BTN_TRI 2 // 18
#define BTN_NOISE 3 // 19
#define BTN_DMC 4 // 20

#define BTN_SHIFT 16
#define BTN_SAVE 17 // 0
//#define BTN_SET 17 // 1
#define BTN_PROGRAM 18 // 2
#define BTN_PATTERN 19 // 3
#define BTN_TRACK 20 // 4
#define BTN_SETTINGS 21 // 5
#define BTN_UP 22 // 6
#define BTN_DOWN 23 // 7

#define MODE_PROGRAM 0
#define MODE_PATTERN 1
#define MODE_TRACK 2
#define MODE_SETTINGS 3
#define MODE_GETVALUE 0x80
#define MODE_TRANSFER 0x40

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

#define button_led_byte(BTN) ((BTN) / 4)
#define button_led_shift(BTN) (((BTN) % 4) * 2)
#define button_led_set(BTN, VAL) (button_leds[button_led_byte(BTN)] |= (VAL) << button_led_shift(BTN))
#define button_led_get(BTN) ((button_leds[button_led_byte(BTN)] >> button_led_shift(BTN)) & 0b11)
#define button_led_on(BTN) (button_led_set(BTN, 1))
#define button_led_blink(BTN) button_led_set(BTN, 0b10); leds_on(BTN)
#define button_led_off(BTN) (button_leds[button_led_byte(BTN)] &= ~(0b11 << button_led_shift(BTN)))

typedef enum { SESSION_INACTIVE, SESSION_ACTIVE } GetvalueState;

typedef struct {
    Parameter parameter;
    uint8_t button1;
    uint8_t button2;
    GetvalueState state;
} GetvalueSession;

// The current mode (PROGRAM, PATTERN, TRACK or SETTINGS)
extern uint8_t mode;

// Previous inputs
extern uint8_t prev_input[3];
extern uint8_t* button_leds;

extern GetvalueSession ui_getvalue_session;

void ui_handler();
void ui_leds_handler();
uint8_t ui_updown(int8_t* value, int8_t min, int8_t max);
void ui_getvalue_handler();

