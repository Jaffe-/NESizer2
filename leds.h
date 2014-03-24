#pragma once

#define SHIFT_CLK 0b100000
#define SS 0b100
#define SHIFT_DATA 0b1000
#define SHIFT_DATA_bp 1

void setup_leds();
void set_leds(uint8_t);
