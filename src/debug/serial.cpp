#include "serial.h"
#include <SendOnlySoftwareSerial.h>


#ifdef __cplusplus
extern "C" {
#endif


    // config
    #define MOSI 11  // Tx pin


    SendOnlySoftwareSerial mySerial(MOSI);


    // wrappers for c compatibility:

    void serial_debug_setup(long baudrate) { mySerial.begin(baudrate); }

    void serial_print(uint8_t data) { mySerial.write(data); }


#ifdef __cplusplus
}
#endif
