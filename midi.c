#include <avr/io.h>
#include <avr/pgmspace.h>
#include "midi.h"
#include "leds.h"

#define BUFFER_SIZE 32
static uint8_t buffer[BUFFER_SIZE];
static uint8_t buffer_write_pos = 0;
static uint8_t buffer_read_pos = 0;

// Length of messages, excluding the status byte
static const uint8_t message_lengths[] PROGMEM = {
    2, 2, 2, 2, 1, 1, 2, 0,
    1, 1, 2, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

static uint8_t buffer_written = 0;
static uint8_t buffer_read = 0;

uint8_t midi_buffer_read_byte()
{
    uint8_t value = buffer[buffer_read_pos++];
   
    if (buffer_read_pos == BUFFER_SIZE) buffer_read_pos = 0;

    buffer_read++;

    // If we have caught up with the number of writes, reset both counters (to keep them from
    // overflowing)
    if (buffer_read == buffer_written) {
	buffer_read = 0;
	buffer_written = 0;
    }

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
    while (UCSR0A & (1 << RXC0)) {
	buffer[buffer_write_pos++] = UDR0;
	
	if (buffer_write_pos == BUFFER_SIZE) 
	    buffer_write_pos = 0;
	
	buffer_written++;
    }  
}

static inline uint8_t message_length(uint8_t command)
{
    return pgm_read_byte(&message_lengths[command]);
}

static inline uint8_t is_status_byte(uint8_t byte)
{
    return (byte & 0x80) != 0;
}

static inline uint8_t get_command(uint8_t status) 
{
    if ((status & 0xF0) != 0xF0) 
	return (status >> 4) & 0x07;
    else
	return (status & 0x0F) + 0x08;
}

static inline uint8_t get_channel(uint8_t status)
{
    return status & 0x0F;
}

uint8_t midi_is_channel_message(uint8_t command)
{
    return command < 8;
}

uint8_t midi_buffer_bytes_remaining()
/* Returns the raw unread buffer length */
{
    return buffer_written - buffer_read;
}

uint8_t midi_buffer_nonempty()
/* Returns 1 if the buffer has unread messages */
{
    return ((buffer_read < buffer_written) && (buffer_read <= buffer_written - message_length(get_command(buffer[buffer_read_pos]))));
}

MIDIMessage midi_buffer_read()
/* Gets the next message from the buffer and puts the data in a struct object */
{
    MIDIMessage msg = {0};

    uint8_t status;
    // If for some reason the read position doesn't point to the first byte of a message,
    // skip to the next one.
    while (!((status = midi_buffer_read_byte()) & 0x80));

    msg.command = get_command(status);

    if (midi_is_channel_message(msg.command)) 
	msg.channel = get_channel(status);

    // Get first databyte, if any
    if (message_length(msg.command) > 0)
	msg.data1 = midi_buffer_read_byte();
    
    // Check the command to see if there is one or two bytes following the status byte
    if (message_length(msg.command) > 1)
	msg.data2 = midi_buffer_read_byte();
        
    return msg;
}
