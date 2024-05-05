#include "ringbuffer.h"
#include "serial.h"


#ifdef __cplusplus
extern "C" {
#endif


    // constructor
    DebugBuffer::DebugBuffer(int s)
    {
        m_ringSize = s;
        m_buffer = new uint8_t[m_ringSize];
        this->initBuffer();
    }


    // destructor
    DebugBuffer::~DebugBuffer()
    {
        delete [] m_buffer;
    }


    // initialize debug buffer (not sure this is necessary)
    void DebugBuffer::initBuffer()
    {
        for (int i = 0; i < m_ringSize; i++) {
            m_buffer[i] = 0x0;
        }
    }


    // push byte into buffer, move to next load index and increase print queue size
    void DebugBuffer::loadByte(uint8_t data)
    {
        if (m_queueSize < m_ringSize) {
            m_buffer[m_loadIndex] = data;
            if (m_loadIndex + 1 == m_ringSize) {
                m_loadIndex = 0;
            } else {
                m_loadIndex++;
            }
            m_queueSize++;
        }
    }


    // print byte from buffer to console, move to next print index and decrease print queue size
    void DebugBuffer::printByte()
    {
        if (m_queueSize > 0) {
            // std::cout << m_buffer[m_printIndex];
            serial_print(m_buffer[m_printIndex]);
            if (m_printIndex + 1 == m_ringSize) {
                m_printIndex = 0;
            } else {
                m_printIndex++;
            }
            m_queueSize--;
        }
    }


#ifdef __cplusplus
}
#endif
