#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "parameter/parameter.h"


struct midi_command {
    uint8_t cc;
    enum parameter_id parameter;
    uint8_t cc_toggle;
    int8_t* stashed_state;  // Saved paramter target value
    bool* stash_active;  // Is the state stored
};

struct state_toggle {
    int8_t state;
    bool stashed;
};

void midi_init(void);
void control_change(uint8_t midi_chn, uint8_t data1, uint8_t data2);
int8_t midi_command_get_cc(uint8_t chn, uint8_t data1 );
struct midi_command midi_command_get(uint8_t chn, int8_t index );
uint8_t get_target_value(struct parameter parameter, int8_t cc_value );
void state_toggle_init (struct state_toggle *arg, size_t ln );