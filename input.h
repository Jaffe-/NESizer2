#pragma once

#define ANALOG_INPUT 0b00100000

uint8_t input[24];

uint8_t input_analog_value;

void input_refresh();
void input_analog_refresh();
void input_setup();
