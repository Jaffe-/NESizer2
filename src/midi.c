/*
  NESIZER
  MIDI Interpreter 
  
  (c) Johan Fjeldtvedt

  Interprets MIDI messages and acts accordingly.
*/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "midi.h"
#include "midi_io.h"
#include "envelope.h"
#include "lfo.h"
#include "ui.h"
#include "ui_sequencer.h"
#include "modulation.h"
#include "leds.h"
#include "portamento.h"
#include "assigner.h"
#include "sample.h"

typedef enum {STATE_MESSAGE, STATE_SYSEX, STATE_TRANSFER} midi_state_e;

uint8_t midi_channels[5];
uint8_t midi_notes[5];

static midi_state_e state = STATE_MESSAGE;
static uint8_t sysex_command;

static inline void interpret_message();
static inline void transfer();
static inline void sysex();

static inline void initiate_transfer();
static inline uint8_t get_midi_channel(uint8_t channel);

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
    MIDIMessage msg = {0};
    if (midi_io_read_message(&msg)) {

      if (midi_is_channel_message(msg.command)) {
	for (uint8_t i = 0; i < 5; i++) {
	  if (get_midi_channel(i) == msg.channel) {
	    switch (msg.command) {
	      
	    case MIDI_CMD_NOTE_ON:
	      midi_notes[i] = msg.data1;
	      play_note(i, msg.data1);
	      break;
	      
	    case MIDI_CMD_NOTE_OFF:
	      if (midi_notes[i] == msg.data1) {
		midi_notes[i] = 0;
		stop_note(i);
	      }
	      break;
	      
	    case MIDI_CMD_PITCH_BEND:
	      if (i < 4) {
		mod_pitchbend_input[i] = ((uint16_t)msg.data1) | ((uint16_t)msg.data2) << 7;
	      }
	      break;
	    }
	  }
	}
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
      sample_new(sample_number);

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
	  sample_write_serial(val);
	  midi_transfer_progress = (sample.bytes_done << 4) / sample.size;
	}
      }
    }
  }
}

static inline uint8_t get_midi_channel(uint8_t channel)
{
  return midi_channels[channel] - 1;
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
