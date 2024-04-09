#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    void serial_debug_setup();
    void serial_debug_handler(uint8_t *buffer);
    void serial_debug(uint8_t data);

#ifdef __cplusplus
}
#endif
