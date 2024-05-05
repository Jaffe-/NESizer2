#pragma once

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


    class DebugBuffer {
    public:
        DebugBuffer(int s);
        ~DebugBuffer();

    private:
        int m_ringSize;
        uint8_t *m_buffer;
        int m_loadIndex = 0;
        int m_printIndex = 0;
        int m_queueSize = 0;

    public:
        void loadByte(uint8_t data);
        void printByte();
        int getRingSize() { return m_ringSize; }
        int getQueueSize() { return m_queueSize; }

    private:
        void initBuffer();
    };


#ifdef __cplusplus
}
#endif
