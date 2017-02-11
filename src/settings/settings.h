#pragma once

#include <stdint.h>

enum settings_id {
    MIDI_CHN = 0,
    PROGRAMMER_SELECTED_PATCH = 5,
    ASSIGNER_UPPER_MODE,
    ASSIGNER_LOWER_MODE,
    ASSIGNER_SPLIT,
    SEQUENCER_SELECTED_SEQ,
    SEQUENCER_EXT_CLK
};

int8_t settings_read(enum settings_id id);
void settings_write(enum settings_id id, int8_t value);
void settings_init(void);
