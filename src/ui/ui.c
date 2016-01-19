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



  General user interface routines

  Contains the UI handler which checks button presses and transfers control to 
  the corresponding mode handlers, and the LED refresh handler.
*/


#include "ui/ui.h"
#include "ui/ui_sequencer.h"
#include "ui/ui_programmer.h"
#include "ui/ui_assigner.h"
#include "ui/ui_settings.h"
#include "io/leds.h"
#include "io/input.h"
#include "midi/midi.h"
#include "modulation/modulation.h"

#define BLINK_CNT 30

enum mode mode = MODE_PROGRAMMER;

uint8_t prev_input[3] = {0};

/* Pointer to array holding LED states for chosen mode */
uint8_t* button_leds = programmer_leds;

static inline uint8_t remove_flags(uint8_t mode)
{
  return mode & 0x0F;
}

void ui_handler(void)
/*
  Top level user interface handler. Checks whether one of the
  mode buttons have been pressed, and transfers control to 
  the corresponding function.
*/
{
  if (mode & MODE_GETVALUE)
    ui_getvalue_handler();

  else {
    if (button_pressed(BTN_PROGRAMMER)) {
      mode = MODE_PROGRAMMER;
      button_leds = programmer_leds;
      button_led_on(BTN_PROGRAMMER);
    }
    else if (button_pressed(BTN_SEQUENCER)) {
      mode = MODE_SEQUENCER;
      button_leds = sequencer_leds;
      button_led_on(BTN_SEQUENCER);
    }
    else if (button_pressed(BTN_ASSIGNER)) {
      mode = MODE_ASSIGNER;
      button_leds = assigner_leds;
      button_led_on(BTN_ASSIGNER);
    }
    else if (button_pressed(BTN_SETTINGS)) {
      mode = MODE_SETTINGS;
      button_leds = settings_leds;
      button_led_on(BTN_SETTINGS);
    }
    
    switch (mode) {
      case MODE_PROGRAMMER:
	button_led_on(BTN_PROGRAMMER);
	programmer();
	break;

      case MODE_SEQUENCER:
	//sequencer();
	button_led_on(BTN_SEQUENCER);
	break;
      case MODE_ASSIGNER:
	button_led_on(BTN_ASSIGNER);
	break;  // not implemented yet!
      case MODE_SETTINGS:
	settings();
	button_led_on(BTN_SETTINGS);
	break;
    }
  }
      
  // Save current button states
  prev_input[0] = input[0];
  prev_input[1] = input[1];
  prev_input[2] = input[2];
}


#define LED_STATE_OFF 0
#define LED_STATE_ON 1
#define LED_STATE_BLINK 2

void ui_leds_handler(void)
/* 
   Handles the updating of all button LEDs.
   If a button's value is 0xFF, it is on, and
   if it's 0, it's off. Otherwise the value is the
   current value of a counter which is used to blink
   the LEDs. 
*/
{
  static uint8_t counter;
  
  for (uint8_t i = 0; i < 24; i++) {
    if (button_led_get(i) == LED_STATE_OFF)
      leds_off(i);

    else if (button_led_get(i) == LED_STATE_ON)
      leds_on(i);
    
    else  {
      if (counter == BLINK_CNT) {
	leds_toggle(i);
      }
    }
  }
  if (counter++ == BLINK_CNT)
    counter = 0;
}


/* 
   Functions used by the user interface handlers
*/

#define WAIT_CNT 60
#define SPEED_CNT 10

uint8_t ui_updown(int8_t* value, int8_t min, int8_t max)
/* Handles up/down buttons when selecting values */
{
  static uint8_t wait_count = 0;
  static uint8_t speed_count = 0;

  if ((button_on(BTN_UP) && *value < max) || (button_on(BTN_DOWN) && *value > min)) {
    // If the button was just pressed, increase/decrease one step and start the wait counter
    // (to avoid the value rapidly changing if the button is held for too long)
    if (button_pressed(BTN_UP)) {
      (*value)++;
      wait_count = 0;
      return 1;
    }
	
    else if (button_pressed(BTN_DOWN)) {
      (*value)--;
      wait_count = 0;
      return 1;
    }
	
    // If the button was not just pressed, increase the wait counter and see if 
    // we can start to increase the value continuously
    else {
      if (wait_count == WAIT_CNT) {
	if (speed_count++ == SPEED_CNT) {
	  speed_count = 0;
	  *value = (button_on(BTN_UP)) ? *value + 1 : *value - 1;
	  return 1;
	}
      }
      else 
	wait_count++;
    }
  }
  return 0;
}

struct getvalue_session ui_getvalue_session = {.state = SESSION_INACTIVE};

void ui_getvalue_handler()
/* Handles getting a parameter value. A new getvalue-session is initiated by
   using ui_getvalue_session. This structure holds the current parameter to
   be changed, which buttons to blink, and the state. 
*/
{
  static int8_t value;
    
  // If the state just changed to GETVALUE, set the value to the parameter's value
  // and last pot value to the current. 
  if (ui_getvalue_session.state == SESSION_INACTIVE) {
    if (ui_getvalue_session.parameter.type == INVRANGE) 
      value = ui_getvalue_session.parameter.max - *ui_getvalue_session.parameter.target;
    else
      value = *ui_getvalue_session.parameter.target;
	
    button_led_blink(ui_getvalue_session.button1);
    
    if (ui_getvalue_session.button2 != 0xFF) {
      button_led_blink(ui_getvalue_session.button2 & 0x7F);

      if ((ui_getvalue_session.button2 & 0x80) != 0) {
	button_led_blink(BTN_SAVE);
      }
    }

    ui_getvalue_session.state = SESSION_ACTIVE;
  }

  // Handle up and down buttons
  ui_updown(&value, ui_getvalue_session.parameter.min, ui_getvalue_session.parameter.max);

  // When SET is pressed, store the new value in the parameter and disable LED blinking.
  // If type is VALTYPE_INVRANGE, the value is inverted. 

  if (button_pressed(BTN_SET)) {
    if (ui_getvalue_session.parameter.type == INVRANGE)
      *ui_getvalue_session.parameter.target = ui_getvalue_session.parameter.max - value;
    else
      *ui_getvalue_session.parameter.target = value;
	
    button_led_off(ui_getvalue_session.button1);
    
    if (ui_getvalue_session.button2 != 0xFF) {
      button_led_off(ui_getvalue_session.button2 & 0x7F);
    }
    
    ui_getvalue_session.state = SESSION_INACTIVE;
	
    mode &= 0x7F;
  }	    

  if (ui_getvalue_session.parameter.min < 0) {
    if (value < 0) {
      leds_7seg_set(3, LEDS_7SEG_MINUS);
      leds_7seg_set(4, -value);
    }
    else {
      leds_7seg_clear(3);
      leds_7seg_set(4, value);
    }
  }
  else 
    leds_7seg_two_digit_set(3, 4, value);
  

}
