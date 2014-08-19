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

#define BTN_TST 0
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

#define button_pressed(BTN) (input[BTN] == 1 && prev_input[BTN] == 0)
#define button_depressed(BTN) (input[BTN] == 0 && prev_input[BTN] == 1)

void ui_handler();
void ui_leds_handler();
uint8_t prev_input[24];
uint8_t button_leds[24];
