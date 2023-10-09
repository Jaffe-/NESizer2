/*
  Copyright 2014-2015 Johan Fjeldtvedt

  This file is part of NESIZER.

  NESIZER is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  NESIZER is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NESIZER.  If not, see <http://www.gnu.org/licenses/>.



  MIDI low level I/O

  Performs the low level functionality of receiving MIDI input. Receiving MIDI
  data is performed by the USART module of the Atmega microcontroller. Incoming
  data is read into a ring buffer.
*/


#include <avr/io.h>
#include <avr/interrupt.h>
#include "midi.h"
#include "ringbuffer.h"

#define BUFFER_SIZE 8

/* Message ring buffer */
static struct ring_buffer input_buffer;
static struct ring_buffer output_buffer;

/* Length of messages, excluding the status byte */
static const uint8_t message_lengths[] = {
    2, 2, 2, 2, 1, 1, 2, 0, // channel messages
    0, 1, 2, 1              // sysex messages
};


/* Internal functions */

static inline uint8_t message_length(uint8_t command);
static inline uint8_t is_status_byte(uint8_t byte);
static inline uint8_t get_command(uint8_t status);
static inline uint8_t get_channel(uint8_t status);


/* Public functions */

void midi_io_setup(void)
{
    // Enable USART receiver and set frame to 8 data bits
    UBRR0H = 0;
    // Period: 20 MHz / (16 * 31250) = 39
    UBRR0L = 39;

    UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
    UCSR0C = (0b11 << UCSZ00);
}

ISR(USART_RX_vect)
{
    // If the RXC0 bit in UCSR0A is set, there is unread data in the receive
    // register.
    while (UCSR0A & (1 << RXC0)) {
        ring_buffer_write(&input_buffer, UDR0);
    }
}

/*
   Task handler for putting incoming MIDI data in buffer
*/
void midi_io_handler(void)
{
    if (ring_buffer_bytes_remaining(&output_buffer) > 0) {
        while (!(UCSR0A & (1 << UDRE0)));
        UDR0 = ring_buffer_read(&output_buffer);
    }
}

uint8_t midi_io_read_byte(void)
{
    return ring_buffer_read(&input_buffer);
}

uint8_t midi_io_bytes_remaining(void)
{
    return ring_buffer_bytes_remaining(&input_buffer);
}

void midi_io_write_byte(uint8_t value)
{
    ring_buffer_write(&output_buffer, value);
}

uint8_t midi_io_buffer_nonempty(void)
{
    uint8_t remaining = ring_buffer_bytes_remaining(&input_buffer);
    return ((remaining >= 1) && (remaining > message_length(get_command(ring_buffer_peek(&input_buffer)))));
}

/* Gets the next message from the buffer and puts the data in a struct object */
uint8_t midi_io_read_message(struct midi_message *msg)
{
    uint8_t status;
    // If for some reason the read position doesn't point to the first byte of a message,
    // skip to the next one.
    if (!((status = ring_buffer_read(&input_buffer)) & 0x80))
        return 0;

    msg->command = get_command(status);

    if (midi_is_channel_message(msg->command))
        msg->channel = get_channel(status);

    // Get first databyte, if any
    if (message_length(msg->command) > 0)
        msg->data1 = ring_buffer_read(&input_buffer);

    // Check the command to see if there is one or two bytes following the status byte
    if (message_length(msg->command) > 1)
        msg->data2 = ring_buffer_read(&input_buffer);

    return 1;
}

void midi_io_write_message(struct midi_message msg)
{
    uint8_t status;
    if (msg.command < 0x08)
        status = 0x80 | (msg.command << 4) | msg.channel;
    else
        status = 0xF0 | (msg.command - 0x08);
    ring_buffer_write(&output_buffer, status);

    if (message_length(msg.command) > 0)
        ring_buffer_write(&output_buffer, msg.data1);

    if (message_length(msg.command) > 1)
        ring_buffer_write(&output_buffer, msg.data2);
}


/* Internal functions */

static inline uint8_t message_length(uint8_t command)
{
    if (command >= 12)
        return 0;
    else
        return message_lengths[command];
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
