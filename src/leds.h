#pragma once

#include <stdint.h>

// Helper macros
#define leds_row(BTN) ((BTN) / 8)
#define leds_col(BTN) ((BTN) % 8)

#define leds_on(BTN) (leds[leds_row(BTN)] |= 1 << leds_col(BTN))
#define leds_off(BTN) (leds[leds_row(BTN)] &= ~(1 << leds_col(BTN)))
#define leds_toggle(BTN) (leds[leds_row(BTN)] ^= 1 << leds_col(BTN))

#define LEDS_7SEG_DOT 16
#define LEDS_7SEG_MINUS 17

// Global variables
extern uint8_t leds[5];
extern uint8_t row_mirror;

// Functions
void leds_refresh();
void leds_7seg_set(uint8_t row, uint8_t value);
void leds_7seg_two_digit_set(uint8_t row1, uint8_t row2, uint8_t value);
void leds_7seg_two_digit_set_hex(uint8_t rwo1, uint8_t row2, uint8_t value);
void leds_7seg_dot_on(uint8_t row);
void leds_7seg_dot_off(uint8_t row);
void leds_7seg_clear(uint8_t row);

