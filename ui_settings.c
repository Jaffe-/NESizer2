#include <avr/io.h>
#include <avr/pgmspace.h>
#include "parameter.h"
#include "ui_settings.h"
#include "midi.h"
#include "input.h"
#include "leds.h"
#include "ui.h"
#include "midi_interpreter.h"
#include "patch.h"
#include "sample.h"
#include "memory.h"
#include "leds.h"

#define BTN_MIDI_CHN 5
#define BTN_PATCH_CLEAR 8
#define BTN_PATTERN_CLEAR 9
#define BTN_SAMPLE_FORMAT 11
#define BTN_TEST 12
#define BTN_MEMCLEAN 13
#define BTN_TEST2 14

#define SIZE(ARR) (sizeof(ARR) / sizeof(ARR[0]))

static inline void toplevel();

void settings()
{
    toplevel();
}

static inline void toplevel()
{    
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
				   .type = VALTYPE_RANGE,
				   .min = 0,
				   .max = 16};
	    ui_getvalue_session.parameter = parameter;
	    ui_getvalue_session.button1 = BTN_MIDI_CHN;
	    ui_getvalue_session.button2 = chn;
	    mode |= MODE_GETVALUE;
	}
    }
}
