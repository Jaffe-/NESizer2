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



  Settings user interface

  Handles the user interface when in settings mode.
*/


#include <avr/io.h>
#include <avr/pgmspace.h>
#include "parameter/parameter.h"
#include "ui_settings.h"
#include "midi/midi.h"
#include "ui.h"
#include "midi/midi.h"
#include "patch/patch.h"
#include "sample/sample.h"
#include "io/leds.h"
#include "io/input.h"
#include "io/memory.h"
#include "io/2a03.h"


#define BTN_MIDI_CHN 5
#define BTN_BLOCKSTATS 6
#define BTN_PATCH_CLEAR 8
#define BTN_PATCH_FORMAT 9
#define BTN_SAMPLE_DELETE 10
#define BTN_SAMPLE_FORMAT 11
#define BTN_MEMCLEAN 13
#define BTN_CLOCKDIV 7

#define SIZE(ARR) (sizeof(ARR) / sizeof(ARR[0]))

enum state {
  STATE_TOPLEVEL,
  STATE_MIDI_CHANNEL
};

static enum state state = STATE_TOPLEVEL;

static inline void toplevel(void);

uint8_t settings_leds[6];
uint8_t assigned_midi_chn[5];

int8_t assign_midi_chn;
uint8_t assign_chn;

void settings(void)
{
  if (state == STATE_TOPLEVEL)
    toplevel();
  else if (state == STATE_MIDI_CHANNEL) {
    // If we're in this state, it means that a MIDI channel has been entered
    // by the user. We now need to subscribe the channel to the new MIDI channel and
    // unsubscribe from the previous one, if any.

    if (assigned_midi_chn[assign_chn] > 0)
      midi_channel_unsubscribe(assigned_midi_chn[assign_chn] - 1, assign_chn);
    midi_channel_subscribe(assign_midi_chn - 1, assign_chn);
    assigned_midi_chn[assign_chn] = assign_midi_chn;
    state = STATE_TOPLEVEL;
  }
}

static inline void toplevel(void)
{    
  static uint8_t cur_index = 0;
  static uint8_t index_state;

  leds_7seg_two_digit_set(3, 4, cur_index);
  if (index_state)
    leds_7seg_dot_on(3);
  else
    leds_7seg_dot_off(3);

  int8_t last_index = cur_index;
  ui_updown(&cur_index, 0, 99);
  if (cur_index != last_index) {
    index_state = sample_occupied(cur_index);
  }
    
  if (button_pressed(BTN_MEMCLEAN))
    memory_clean();

  if (button_pressed(BTN_PATCH_FORMAT)) {
    for (uint8_t i = 0; i < 100; i++)
      patch_initialize(i);
  }

  if (button_pressed(BTN_PATCH_CLEAR)) {
//	patch_initialize();
  }

  if (button_pressed(BTN_SAMPLE_DELETE)) {
    sample_delete(cur_index);
  }

  if (button_on(BTN_CLOCKDIV))
    leds_7seg_two_digit_set(3, 4, io_clockdiv);
    
  if (button_on(BTN_MIDI_CHN)) {
    uint8_t chn = 0xFF;
    for (uint8_t i = 0; i < 5; i++) {
      if (button_pressed(i)) {
	chn = i;
	break;
      }
    }

    if (chn != 0xFF) {
      assign_chn = chn;
      assign_midi_chn = assigned_midi_chn[chn];
      struct parameter parameter = {.target = &assign_midi_chn,
				    .type = RANGE,
				    .min = 0,
				    .max = 16};
      getvalue.parameter = parameter;
      getvalue.button1 = BTN_MIDI_CHN;
      getvalue.button2 = chn;
      getvalue.previous_mode = mode;
      mode = MODE_GETVALUE;

      state = STATE_MIDI_CHANNEL;
    }
  }
}
