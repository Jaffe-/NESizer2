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


#pragma once

#define MIDI_CMD_NOTE_OFF 0
#define MIDI_CMD_NOTE_ON 1
#define MIDI_CMD_AFTERTOUCH 2
#define MIDI_CMD_CONTROL_CHANGE 3
#define MIDI_CMD_PATCH_CHANGE 4
#define MIDI_CMD_CHANNEL_PRESSURE 5
#define MIDI_CMD_PITCH_BEND 6

#define MIDI_CMD_SYSEX 0x08
#define MIDI_CMD_TIMECODE 0x09
#define MIDI_CMD_SONGPOS 0x0A
#define MIDI_CMD_SONGSEL 0x0B
#define MIDI_CMD_TUNEREQUEST 0x0E
#define MIDI_CMD_SYSEX_END 0x0F

#define MIDI_CMD_CLOCK 0x10
#define MIDI_CMD_START 0x12
#define MIDI_CMD_CONTINUE 0x13
#define MIDI_CMD_STOP 0x14
#define MIDI_CMD_ACTIVESENSE 0x16
#define MIDI_CMD_RESET 0x17

#define midi_is_channel_message(cmd) ((cmd) < 8)

struct midi_message {
    uint8_t command;
    uint8_t channel;
    uint8_t data1;
    uint8_t data2;
};

void midi_io_setup(void);
void midi_io_handler(void);
uint8_t midi_io_buffer_nonempty(void);
uint8_t midi_io_read_message(struct midi_message *msg);
uint8_t midi_io_read_byte(void);
uint8_t midi_io_bytes_remaining(void);
