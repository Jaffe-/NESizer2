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



  MIDI interpreter

  Interprets MIDI messages and acts accordingly.
*/


#include <stdint.h>
#include "midi.h"
#include "io/midi.h"
#include "envelope/envelope.h"
#include "lfo/lfo.h"
#include "ui/ui.h"
#include "ui/ui_sequencer.h"
#include "modulation/modulation.h"
#include "io/leds.h"
#include "portamento/portamento.h"
#include "assigner/assigner.h"
#include "sample/sample.h"

typedef enum {STATE_MESSAGE, STATE_SYSEX, STATE_TRANSFER} midi_state_e;

uint8_t midi_notes[5];

static midi_state_e state = STATE_MESSAGE;
static uint8_t sysex_command;

static inline void interpret_message();
static inline void transfer();
static inline void sysex();

static inline void initiate_transfer();
static inline uint8_t get_midi_channel(uint8_t channel);

struct midi_channel midi_channels[5];

struct midi_channel* midi_channel_get(uint8_t midi_chn)
{
  for (uint8_t i = 0; i < 5; i++) {
    if (midi_channels[i].channel == midi_chn)
      return &midi_channels[i];
  }
  return 0;
}

struct midi_channel* midi_channel_allocate(uint8_t midi_chn)
{
  for (uint8_t i = 0; i < 5; i++) {
    if (midi_channels[i].listeners_count == 0) {
      midi_channels[i].channel = midi_chn;
      midi_channels[i].note_list_length = 0;
      for (uint8_t j = 0; j < MIDI_NOTE_LIST_MAX; j++)
	midi_channels[i].note_list[j] = 0;
      return &midi_channels[i];
    }
  }
}

/* Make an APU channel listen to the given MIDI channel */
void midi_channel_subscribe(uint8_t midi_chn, uint8_t chn)
{
  struct midi_channel* midi_channel;

  if (!(midi_channel = midi_channel_get(midi_chn))) {
    midi_channel = midi_channel_allocate(midi_chn);
  }

  midi_channel->listeners |= (1 << chn);
  midi_channel->listeners_count++;
}

void midi_channel_unsubscribe(uint8_t midi_chn, uint8_t chn)
{
  struct midi_channel* midi_channel = midi_channel_get(midi_chn);

  midi_channel->listeners &= ~(1 << chn);
  midi_channel->listeners_count--;
}

void midi_channel_note_on(struct midi_channel* midi_channel, uint8_t note)
{
  if (midi_channel->note_list_length == MIDI_NOTE_LIST_MAX)
    return;

  if (midi_channel->note_list_length == 0) {
    midi_channel->note_list[0] = note;
  }
  else {
    uint8_t next, set = 0;
    for (uint8_t i = 0; i < midi_channel->note_list_length + 1; i++) {
      if (!set && note < midi_channel->note_list[i]) {
	set = 1;
	next = midi_channel->note_list[i];
	midi_channel->note_list[i++] = note;
      }
      if (set) {
	uint8_t temp = midi_channel->note_list[i];
	midi_channel->note_list[i] = next;
	next = temp;
      }
    }
    if (!set)
      midi_channel->note_list[midi_channel->note_list_length] = note;
  }
  midi_channel->note_list_length++;
}

void midi_channel_note_off(struct midi_channel* midi_channel, uint8_t note)
{
  uint8_t* p = midi_channel->note_list;
  for (uint8_t i = 0; i < midi_channel->note_list_length; i++) {
    if (midi_channel->note_list[i] == note)
      i += 1;
    *(p++) = midi_channel->note_list[i];
  }
  midi_channel->note_list_length--;
}

void midi_channel_put(struct midi_message* msg)
{
  struct midi_channel* midi_chn;
  if (!(midi_chn = get_midi_channel(msg->channel)))
    return;

  switch (msg->command) {
  case MIDI_CMD_NOTE_ON:
    midi_channel_note_on(midi_chn, msg->data1);
    break;

  case MIDI_CMD_NOTE_OFF:
    midi_channel_note_off(midi_chn, msg->data1);
    break;

  case MIDI_CMD_PITCH_BEND:
    for (uint8_t i = 0; i < 5; i++) {
      if (midi_chn->listeners & (1 << i)) {
	if (i < 3)
	  mod_pitchbend_input[i] = ((uint16_t)msg->data1) | ((uint16_t)msg->data2) << 7;
	else if (i == 4)
	  mod_pitchbend_input[i] = msg->data2 >> 3;
      }
    }
    break;
  }
}

void midi_handler()
{
  switch (state) {
  case STATE_MESSAGE:
    interpret_message(); break;
  case STATE_SYSEX:
    sysex(); break;
  case STATE_TRANSFER:
    transfer(); break;
  default: break;
  }
}

static inline void interpret_message()
{
  while (midi_io_buffer_nonempty()) {
    struct midi_message msg = {0};
    if (midi_io_read_message(&msg)) {

      if (midi_is_channel_message(msg.command)) {
	midi_channel_put(&msg);
      }
 
      else {
	switch (msg.command) {
	case MIDI_CMD_SYSEX:
	  state = STATE_SYSEX;
	  break;
	  
	case MIDI_CMD_TIMECODE:
	  break;
		
	case MIDI_CMD_SONGPOS:
	  break;
	    
	case MIDI_CMD_SONGSEL:
	  break;
		
	case MIDI_CMD_TUNEREQUEST:
	  break;
		
	case MIDI_CMD_SYSEX_END:
	  break;
		
	case MIDI_CMD_CLOCK:
	  break;
		
	case MIDI_CMD_START:
	  break;
		
	case MIDI_CMD_CONTINUE:
	  break;
		
	case MIDI_CMD_STOP:
	  break;
		
	case MIDI_CMD_ACTIVESENSE:
	  break;
		
	case MIDI_CMD_RESET:
	  break;
	}
      }
    }
  }
}

#define SYSEX_STOP 0xF7
#define SYSEX_CMD_SAMPLE_LOAD 1
#define SYSEX_CMD_FIRMWARE_LOAD 2

uint8_t midi_transfer_progress = 0;
static uint8_t dmc_state;

static struct sample sample;

static inline void sysex()
{
  // When the state just changed, we need to look at the first few bytes
  // to determine what sysex message we're dealing with
  if (midi_io_bytes_remaining() >= 6) {
    sysex_command = midi_io_read_byte();

    if (sysex_command == SYSEX_CMD_SAMPLE_LOAD) {

      // Read sample descriptor and store in sample object
      uint8_t sample_number = midi_io_read_byte();
      sample.type = midi_io_read_byte();
      sample.size = midi_io_read_byte();
      sample.size |= (uint32_t)midi_io_read_byte() << 7;
      sample.size |= (uint32_t)midi_io_read_byte() << 14;
      sample_new(&sample, sample_number);

      initiate_transfer();
    }

    else if (sysex_command == SYSEX_CMD_FIRMWARE_LOAD) {
      // To do
      state = STATE_TRANSFER;
    }
  }

}

static inline void transfer()
/*
  Handlers transfering of data via MIDI
*/
{
  static uint8_t nibble_flag = 0;
  static uint8_t temp_val = 0;
  
  while (midi_io_bytes_remaining() >= 1) {
    uint8_t val = midi_io_read_byte();

    if (val == SYSEX_STOP) {
      mode &= ~MODE_TRANSFER;
      state = STATE_MESSAGE;
      dmc.enabled = dmc_state;
    }

    else if ((val & 0x80) == 0) {
      if (sysex_command == SYSEX_CMD_FIRMWARE_LOAD) {
	if (nibble_flag == 0) 
	  temp_val = val << 4;
	else {
	  temp_val |= val;
	  // TODO: Write data to firmware memory
	}
	nibble_flag ^= 1;
      }
      
      else if (sysex_command == SYSEX_CMD_SAMPLE_LOAD) {
	if (sample.bytes_done < sample.size) {
	  sample_write_serial(&sample, val);
	  midi_transfer_progress = (sample.bytes_done << 4) / sample.size;
	}
      }
    }
  }
}

static inline void initiate_transfer()
{
  // Set UI mode to transfer (which turns the button LEDs into a status bar
  // for the duration of the transfer)
  mode |= MODE_TRANSFER;

  // Disable DMC
  dmc_state = dmc.enabled;
  dmc.enabled = 0;

  state = STATE_TRANSFER;
  midi_transfer_progress = 0;
}
