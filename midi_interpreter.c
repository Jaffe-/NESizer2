#include <avr/io.h>
#include <avr/pgmspace.h>
#include "midi_interpreter.h"
#include "midi.h"
#include "envelope.h"
#include "lfo.h"
#include "sequencer.h"
#include "modulation.h"
#include "leds.h"

uint8_t midi_channels[5];
uint8_t notes[5];

static inline uint16_t midi_note_to_period(uint8_t midi_note)
{
    uint8_t note = midi_note % 12;
    uint8_t octave = (midi_note - note) / 12 - 2;
    return pgm_read_word(&period_table[octave][note]);
}

static inline uint8_t get_midi_channel(uint8_t channel)
{
    return midi_channels[channel] - 1;
}

static void play_note(uint8_t channel, uint8_t note)
{
    notes[channel] = note;

    uint16_t period = midi_note_to_period(note);

    switch (channel) {
    case CHN_SQ1:
	env1.gate = 1;
	periods[0] = period;
	break;
	
    case CHN_SQ2:
	env2.gate = 1;
	periods[1] = period;
	break;
	
    case CHN_TRI:
	tri.enabled = 1;
	periods[2] = period;
	break;

    case CHN_NOISE:
	env3.gate = 1;
	periods[3] = note - 24;
	break;

    case CHN_DMC:
	sample_load(&dmc.sample, note - 24);
	if (dmc.sample.size != 0)
	    dmc.sample_enabled = 1;
	dmc.sample_loop = 0;
	break;
    }
}

static void stop_note(uint8_t channel)
{
    switch (channel) {
    case CHN_SQ1:
	env1.gate = 0;
	break;
	
    case CHN_SQ2:
	env2.gate = 0;
	break;
	
    case CHN_TRI:
	periods[3] = 0;
	break;

    case CHN_NOISE:
	env3.gate = 0;
	break;

    case CHN_DMC:
	dmc.sample_enabled = 0;
    }
}

#define STATE_MESSAGE 0
#define STATE_SYSEX 1

#define SYSEX_STOP 0b11110111
#define SYSEX_CMD_SAMPLE_LOAD 1

void midi_interpreter_handler()
{
    static uint8_t state = STATE_MESSAGE;
    static uint8_t state_changed = 0;
    static uint8_t sysex_command = 0;
    static Sample sample = {0};

    if (state == STATE_MESSAGE) {
	while (midi_buffer_nonempty()) {
	    MIDIMessage msg = midi_buffer_read();
	    
	    if (msg.command == MIDI_CMD_SYSEX) {
		state = STATE_SYSEX;
		state_changed = 1;
	    }
	    else {
		for (uint8_t i = 0; i < 5; i++) {
		    if (get_midi_channel(i) == msg.channel) {
			switch (msg.command) {
			    
			case MIDI_CMD_NOTE_ON:
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
	}
    }

    else if (state == STATE_SYSEX) {
	// When the state just changed, we need to look at the first few bytes
	// to determine what sysex message we're dealing with
	if (state_changed) {
	    if (midi_buffer_bytes_remaining() >= 6) {
		sysex_command = midi_buffer_read_byte();

		if (sysex_command == SYSEX_CMD_SAMPLE_LOAD) {
		    uint8_t sample_number = midi_buffer_read_byte();
		    sample.type = midi_buffer_read_byte();
		    sample.size = midi_buffer_read_byte();
		    sample.size |= (uint32_t)midi_buffer_read_byte() << 7;
		    sample.size |= (uint32_t)midi_buffer_read_byte() << 14;
		    sample_new(&sample, sample_number);
		}
		
		state_changed = 0;
	    }
	}
	
	if (sysex_command == SYSEX_CMD_SAMPLE_LOAD) {
	    if (sample.bytes_done < sample.size) {
		while(midi_buffer_bytes_remaining() >= 1) {
		    uint8_t val = midi_buffer_read_byte();
		    if ((val & 0x80) == 0) {
			// Simply read buffer and write serially to the sample SRAM
			sample_write_serial(&sample, val);
		    }
		    else {
			state = STATE_MESSAGE;
			break;
		    }
		}
	    }
	    else {
		state = STATE_MESSAGE;
	    }
	}
	
    }
}
