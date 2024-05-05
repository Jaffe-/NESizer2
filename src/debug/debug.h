#pragma once

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


    enum MESSAGE_HEADER { DBG_STOP = 0b10100000, DBG_MIDI = 0b10100001 };  // more debug messages types coming soon

    void debug_message(int message_type, int size, ...);


    void debug_setup();
    void debug_load(uint8_t data);
    void debug_print();


#ifdef __cplusplus
}
#endif
