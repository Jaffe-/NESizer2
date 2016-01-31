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



  Channel note assignment

  Assigns notes from MIDI or button input to sound channels.
*/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "modulation/modulation.h"
#include "portamento/portamento.h"
#include "apu/apu.h"
//#include "lfo/lfo.h"
#include "envelope/envelope.h"
#include "assigner/assigner.h"
#include "sample/sample.h"
#include "modulation/periods.h"
#include "midi/midi.h"

enum arp_mode assigner_arp_mode;
int8_t assigner_arp_range;
int8_t assigner_arp_channel;
int8_t assigner_arp_rate;
int8_t assigner_arp_sync;

enum assigner_mode assigner_mode = MONO;

uint8_t assigned_notes[5];

int8_t midi_note_to_note(uint8_t midi_note)
{
  return (int8_t)midi_note - 24;
}

void play_note(uint8_t channel, uint8_t midi_note)
{
  uint8_t note = midi_note_to_note(midi_note);
  assigned_notes[channel] = midi_note;

  switch (channel) {
  case CHN_SQ1:
    env[0].gate = 1;
    portamento_target_notes[0] = note;
    break;
	
  case CHN_SQ2:
    env[1].gate = 1;
    portamento_target_notes[1] = note;
    break;
	
  case CHN_TRI:
    tri.silenced = 0;
    portamento_target_notes[2] = note;
    break;

  case CHN_NOISE:
    env[2].gate = 1;
    noise_period = note - 24;
    break;

  case CHN_DMC:
    if (sample_occupied(midi_note - 60)) {
      sample_load(&dmc.sample, midi_note - 60);
      if (dmc.sample.size != 0)
	dmc.sample_enabled = 1;
      break;
    }
  }
}

void stop_note(uint8_t channel)
{
  switch (channel) {
  case CHN_SQ1:
    env[0].gate = 0;
    break;
	
  case CHN_SQ2:
    env[1].gate = 0;
    break;
	
  case CHN_TRI:
    tri.silenced = 1;
    break;

  case CHN_NOISE:
    env[2].gate = 0;
    break;

  case CHN_DMC:
    dmc.sample_enabled = 0;
  }

  assigned_notes[channel] = 0;
}

void assigner_handler(void)
{
  for (uint8_t i = 0; i < 5; i++) {
    if (midi_channels[i].listeners_count > 0) {
      for (uint8_t chn = 0; chn < 5; chn++) {
	if (midi_channels[i].listeners & (1 << chn)) {
      //&& assigner_arp_channel != midi_channels[i].channel) {

	  switch (assigner_mode) {

	  case MONO:

	    // Get the lowest note and assign it. Stop previous note if any.
	    if (midi_channels[i].note_list_length > 0) {
	      uint8_t note = midi_channels[i].note_list[0];
	      if (note != assigned_notes[chn]) {
		stop_note(chn);
		play_note(chn, note);
	      }
	    }

	    // If the note list is empty, we know that whatever note is playing
	    // should stop.
	    else if (assigned_notes[chn] != 0)
	      stop_note(chn);
	    break;

	  case POLY1:
	    if (midi_channels[i].note_list_length > chn) {
	      uint8_t note = midi_channels[i].note_list[chn];
	      if (note != assigned_notes[chn]) {
		stop_note(chn);
		play_note(chn, note);
	      }
	    }

	    else if (assigned_notes[chn] != 0)
	      stop_note(chn);
	    break;

	  case POLY2:
	    break;
	  }
	}
      }
    }
  }
}
