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

#define BTN_OK 1
#define BTN_TST 7

#define STATE_TOPLEVEL 0
#define STATE_GETVALUE 1

void programmer_handler();
void programmer_tst();
