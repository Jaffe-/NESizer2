#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    enum DEBUG_MESSAGE_HEADER {
        DBG_STOP = 0b10100000,
        DBG_TEXT,
        DBG_MIDI_NOTE,
        DBG_MIDI_CC,
        DBG_PARAM_EDIT,
        DBG_MIDI_SYSEX
    };  // more debug messages types coming soon

    void debug_setup();
    void debug_load(uint8_t data);
    void debug_print();

    void debug_text_message(const char *msg);
    void debug_byte_message(uint8_t msg_type, uint8_t size, ...);
    void debug_load_word(uint16_t data);
    void debug_load_7dword(uint32_t data);
    uint8_t debug_thru(uint8_t data);
    void debug_stop();

#ifdef __cplusplus
}
#endif
