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
#include "ui_sequencer.h"
#include "modulation.h"
#include "leds.h"
#include "portamento.h"
#include "assigner.h"

#define STATE_MESSAGE 0
#define STATE_SYSEX 1

uint8_t midi_channels[5];
static uint8_t notes[5];

static uint8_t state = STATE_MESSAGE;
static uint8_t state_changed = 0;

static inline uint8_t get_midi_channel(uint8_t channel)
{
  return midi_channels[channel] - 1;
}

static inline void interpret_message()
{
  while (midi_io_buffer_nonempty()) {
    MIDIMessage msg = midi_io_read_message();

    if (midi_is_channel_message(msg.command)) {
      for (uint8_t i = 0; i < 5; i++) {
	if (get_midi_channel(i) == msg.channel) {
	  switch (msg.command) {
			
	  case MIDI_CMD_NOTE_ON:
	    notes[i] = msg.data1;
	    play_note(i, msg.data1);
	    break;
			
	  case MIDI_CMD_NOTE_OFF:
	    if (notes[i] == msg.data1)
	      stop_note(i);
	    break;
			
	  case MIDI_CMD_PITCH_BEND:
	    break;
	  }
	}
      }
    }

    else {
      switch (msg.command) {
      case MIDI_CMD_SYSEX:
	state = STATE_SYSEX;
	state_changed = 1;
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

#define SYSEX_STOP 0b11110111
#define SYSEX_CMD_SAMPLE_LOAD 1

void midi_handler()
{
  static uint8_t sysex_command = 0;
  static Sample sample = {0};
  
  if (state == STATE_MESSAGE) {
    interpret_message();
  }

  else if (state == STATE_SYSEX) {
    // When the state just changed, we need to look at the first few bytes
    // to determine what sysex message we're dealing with
    if (state_changed) {
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
	}
		
	state_changed = 0;
      }
    }
    
    if (sysex_command == SYSEX_CMD_SAMPLE_LOAD) {
      while(midi_io_bytes_remaining() >= 1) {
	uint8_t val = midi_io_read_byte();
	if ((val & 0x80) == 0) {
	  // Read buffer and write serially to the sample SRAM
	  if (sample.bytes_done < sample.size) 
	    sample_write_serial(&sample, val);
	}
	// When the end of sysex message is received, return to message mode
	else if (val == SYSEX_STOP) {
	  state = STATE_MESSAGE;
	  break;
	}
      }
    }	
  }
}
