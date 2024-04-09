#include <SendOnlySoftwareSerial.h>
#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif

    #define MOSI 11  // Tx pin

    SendOnlySoftwareSerial mySerial(MOSI);

    void serial_debug_setup() {
        mySerial.begin(92160);  // "9600" = 7680; "115200" = 92160;
    }

    void serial_debug_handler(uint8_t *buffer) {
        return;
    }

    void serial_print(uint8_t data) {
        mySerial.println(data);
    }


#ifdef __cplusplus
}
#endif
