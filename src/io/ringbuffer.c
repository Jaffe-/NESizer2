#include <stdint.h>
#include "ringbuffer.h"

uint8_t ring_buffer_read(struct ring_buffer *buffer)
{
    uint8_t value = buffer->data[buffer->read_pos];

    if (++buffer->read_pos == SIZE)
        buffer->read_pos = 0;

    return value;
}

void ring_buffer_write(struct ring_buffer *buffer, uint8_t value)
{
    buffer->data[buffer->write_pos] = value;

    if (++buffer->write_pos == SIZE)
        buffer->write_pos = 0;
}

uint8_t ring_buffer_bytes_remaining(const struct ring_buffer *buffer)
{
    if (buffer->write_pos >= buffer->read_pos)
        return buffer->write_pos - buffer->read_pos;
    else
        return SIZE - (buffer->read_pos - buffer->write_pos);
}
