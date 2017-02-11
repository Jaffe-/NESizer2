#include <stdint.h>
#include "io/memory.h"
#include "settings.h"

#define SETTINGS_BASE_ADDRESS 0x80
#define SETTINGS_SIZE (SEQUENCER_EXT_CLK - MIDI_CHN + 1)

int8_t settings_read(enum settings_id id)
{
    return memory_read(SETTINGS_BASE_ADDRESS + id);
}

void settings_write(enum settings_id id, int8_t value)
{
    memory_write(SETTINGS_BASE_ADDRESS + id, value);
}

void settings_init(void)
{
    for (uint8_t i = 0; i < SETTINGS_SIZE; i++) {
        memory_write(SETTINGS_BASE_ADDRESS + i, 0);
    }
}
