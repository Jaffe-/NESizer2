#pragma once

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


    enum MESSAGE_HEADER { DBG_STOP = 0b10100000, DBG_TEXT, DBG_MIDI_NOTE, DBG_MIDI_CC, DBG_PARAM_EDIT };  // more debug messages types coming soon

    void debug_byte_message(uint8_t msg_type, uint8_t size, ...);
    void debug_text_message(const char *msg);

    void debug_setup();
    void debug_load(uint8_t data);
    void debug_print();


#ifdef __cplusplus
}
#endif
