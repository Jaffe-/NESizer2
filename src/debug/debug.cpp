#include "debug.h"
#include "serial.h"
#include "ringbuffer.h"
#include <stdarg.h>


#ifdef __cplusplus
extern "C" {
#endif


    // config
    #define DEBUG_BUFFER_SIZE 128
    #define SERIAL_BAUDRATE 115200


    DebugBuffer debugBuffer{DEBUG_BUFFER_SIZE};


    /*
        debug_message(...) is Variadic Function for creating debug messages with dynamic size.
        it loads bytes to the ring buffer in order to build a packet with a header/instructions for processing on the RX end.

        the first loaded byte will be the message type, such as "midi note", followed by an arbitrary number data bytes.
        a "stop" byte will be appeneded automatically, and the RX MCU can use the headers to decide how to format
        the message before printing to a console. if you just want to see individual bytes, use debug_load()

        specify the message type from MESSAGE_HEADER in debug.h, then specify the number of bytes to be loaded, followed by the bytes in order
    */
    void debug_message(uint8_t msg_type, uint8_t size, ...)
    {
        va_list args;
        va_start(args, size);
            debug_load(msg_type);
            debug_load(size);
            while(size > 0) {
                debug_load((uint8_t)va_arg(args, int));
                size--;
            }
            debug_load(MESSAGE_HEADER::DBG_STOP);
        va_end(args);
    }


    // wrappers for c compatibility:

    void debug_setup() { serial_debug_setup(SERIAL_BAUDRATE); }

    void debug_load(uint8_t data) { debugBuffer.loadByte(data); }

    void debug_print() { debugBuffer.printByte(); }


#ifdef __cplusplus
}
#endif
