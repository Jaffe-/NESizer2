/*
  NESIZER

  MIDI IO functions 

  (c) Johan Fjeldtvedt

  Performs the low level functionality of receiving MIDI input. Receiving MIDI 
  data is performed by the USART module of the Atmega microcontroller. Incoming 
  data is read into a ring buffer. 
*/

#include <avr/io.h>
#include "midi_io.h"

#define BUFFER_SIZE 32

/* Message ring buffer */
static uint8_t buffer[BUFFER_SIZE];
static uint8_t buffer_write_pos = 0;
static uint8_t buffer_read_pos = 0;

/* Length of messages, excluding the status byte */
static const uint8_t message_lengths[] = {
  2, 2, 2, 2, 1, 1, 2, 0,
  0, 1, 2, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};


/* Internal functions */

static inline uint8_t message_length(uint8_t command);
static inline uint8_t is_status_byte(uint8_t byte);
static inline uint8_t get_command(uint8_t status);
static inline uint8_t get_channel(uint8_t status);


/* Public functions */

uint8_t midi_io_read_byte()
{
  uint8_t value = buffer[buffer_read_pos];
   
  if (++buffer_read_pos == BUFFER_SIZE)
    buffer_read_pos = 0;

  return value;
}

void midi_io_setup()
{
  // Enable USART receiver and set frame to 8 data bits
  UBRR0H = 0;
  // Period: 20 MHz / (16 * 31250) = 39
  UBRR0L = 39;

  UCSR0B = (1 << RXEN0);
  UCSR0C = (0b11 << UCSZ00);
}

void midi_io_handler()
/* 
Task handler for putting incoming MIDI data in buffer
*/
{
  // If the RXC0 bit in UCSR0A is set, there is unread data in the receive
  // register. 
  while (UCSR0A & (1 << RXC0)) {
    buffer[buffer_write_pos] = UDR0;
	
    if (++buffer_write_pos == BUFFER_SIZE) 
      buffer_write_pos = 0;	
  }  
}

/*
uint8_t midi_is_channel_message(uint8_t command)
{
  return command < 8;
}
*/

uint8_t midi_io_bytes_remaining()
/* Returns the raw unread buffer length */
{
  if (buffer_write_pos >= buffer_read_pos)
    return buffer_write_pos - buffer_read_pos;
  else 
    return BUFFER_SIZE - (buffer_read_pos - buffer_write_pos);    
}

uint8_t midi_io_buffer_nonempty()
{
  uint8_t remaining = midi_io_bytes_remaining();
  return ((remaining >= 1) && (remaining > message_length(get_command(buffer[buffer_read_pos]))));
}

MIDIMessage midi_io_read_message()
/* Gets the next message from the buffer and puts the data in a struct object */
{
  MIDIMessage msg = {0};

  uint8_t status;
  // If for some reason the read position doesn't point to the first byte of a message,
  // skip to the next one.
  while (!((status = midi_io_read_byte()) & 0x80));

  msg.command = get_command(status);

  if (midi_is_channel_message(msg.command)) 
    msg.channel = get_channel(status);

  // Get first databyte, if any
  if (message_length(msg.command) > 0)
    msg.data1 = midi_io_read_byte();
    
  // Check the command to see if there is one or two bytes following the status byte
  if (message_length(msg.command) > 1)
    msg.data2 = midi_io_read_byte();
        
  return msg;
}


/* Internal functions */

static inline uint8_t message_length(uint8_t command)
{
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

