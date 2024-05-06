#include "serial.h"
#include <SendOnlySoftwareSerial.h>


#ifdef __cplusplus
extern "C" {
#endif


    // config
    #define MOSI 11  // Tx pin


    SendOnlySoftwareSerial debugSerial(MOSI);


    // wrappers for c compatibility:

    void serial_debug_setup(long baudrate) { debugSerial.begin(baudrate); }

    void serial_print(uint8_t data) { debugSerial.write(data); }


#ifdef __cplusplus
}
#endif
