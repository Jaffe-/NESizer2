#include <avr/io.h>
#include <avr/pgmspace.h>
#include "midi.h"

#define BUFFER_SIZE 16
static uint8_t buffer[BUFFER_SIZE];
static uint8_t buffer_write_pos = 0;
static uint8_t buffer_read_pos = 0;

static const uint8_t data_bytes_count[] PROGMEM = {2, 2, 2, 2, 1, 1, 2};

static uint8_t buffer_written = 0;
static uint8_t buffer_read = 0;

static inline uint8_t buffer_read_byte()
{
    uint8_t value = buffer[buffer_read_pos++];
   
    if (buffer_read_pos == BUFFER_SIZE) buffer_read_pos = 0;

    buffer_read++;

    return value;
}

void midi_setup()
{
    // Enable receiver and set frame to 8 data bits
    UBRR0H = 0;
    // 20 MHz / (16 * 31250) = 39
    UBRR0L = 39;

    UCSR0B = (1 << RXEN0);
    UCSR0C = (0b11 << UCSZ00);
}

void midi_handler()
/* Task handler for putting incoming MIDI data in buffer */
{
    if (UCSR0A & (1 << RXC0)) {
	buffer[buffer_write_pos++] = UDR0;
	if (buffer_write_pos == BUFFER_SIZE) buffer_write_pos = 0;
	buffer_written++;
    }  
}

uint8_t midi_buffer_state()
/* Returns 1 if the buffer has unread messages */
{
    return (buffer_read < buffer_written);
}

MIDIMessage midi_buffer_read()
/* Gets the next message from the buffer and puts the data in a struct object */
{
    MIDIMessage msg = {0};
    
    uint8_t status;
    // If for some reason the read position doesn't point to the first byte of a message,
    // skip to the next one.
    while (!((status = buffer_read_byte()) & 0x80));
    
    // Split out the channel and command parts
    msg.command = (status >> 4) & 0x07;
    msg.channel = status & 0x0F;
    

    msg.data1 = buffer_read_byte();
    
    // Check the command to see if there is one or two bytes following the status byte
    if (pgm_read_byte(&data_bytes_count[msg.command]) == 2)
	msg.data2 = buffer_read_byte();
    
    // If we have caught up with the number of writes, reset both counters (to keep them from
    // overflowing)
    if (buffer_read == buffer_written) {
	buffer_read = 0;
	buffer_written = 0;
    }
    
    return msg;
}
