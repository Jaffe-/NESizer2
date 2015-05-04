#include <avr/io.h>
#include <avr/pgmspace.h>
#include "parameter.h"
#include "ui_settings.h"
#include "midi.h"
#include "input.h"
#include "leds.h"
#include "ui.h"
#include "midi.h"
#include "patch.h"
#include "sample.h"
#include "memory.h"
#include "leds.h"
#include "2a03_io.h"

#define BTN_MIDI_CHN 5
#define BTN_BLOCKSTATS 6
#define BTN_PATCH_CLEAR 8
#define BTN_PATCH_FORMAT 9
#define BTN_SAMPLE_DELETE 10
#define BTN_SAMPLE_FORMAT 11
#define BTN_MEMCLEAN 13
#define BTN_CLOCKDIV 7

#define SIZE(ARR) (sizeof(ARR) / sizeof(ARR[0]))

static inline void toplevel();

uint8_t settings_leds[24];

void settings()
{
  toplevel();
}

static inline void toplevel()
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
      Parameter parameter = {.target = &midi_channels[chn],
			     .type = RANGE,
			     .min = 0,
			     .max = 16};
      ui_getvalue_session.parameter = parameter;
      ui_getvalue_session.button1 = BTN_MIDI_CHN;
      ui_getvalue_session.button2 = chn;
      mode |= MODE_GETVALUE;
    }
  }
}
