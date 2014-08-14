#pragma once

// Global variables
uint8_t leds[5];
uint8_t row_mirror;

// Functions
void leds_refresh();
void leds_7seg_set(uint8_t row, uint8_t value);
void leds_7seg_dot_on(uint8_t row);
void leds_7seg_dot_off(uint8_t row);
void leds_7seg_clear(uint8_t row);
