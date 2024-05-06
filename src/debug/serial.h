#pragma once

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


    void serial_debug_setup(long baudrate);
    void serial_print(uint8_t data);


#ifdef __cplusplus
}
#endif
