#pragma once

#define SIZE 32

struct ring_buffer {
    uint8_t data[SIZE];
    uint8_t read_pos;
    uint8_t write_pos;
};

uint8_t ring_buffer_read(struct ring_buffer *buffer);
void ring_buffer_write(struct ring_buffer *buffer, uint8_t value);
uint8_t ring_buffer_bytes_remaining(const struct ring_buffer *buffer);
