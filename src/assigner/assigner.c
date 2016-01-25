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


enum arp_mode assigner_arp_mode;
int8_t assigner_arp_range;
int8_t assigner_arp_channel;
int8_t assigner_arp_rate;
int8_t assigner_arp_sync;

enum assigner_mode assigner_mode = 1;

#define NOTE_LIST_MAX 8

struct {
  uint8_t data[5][NOTE_LIST_MAX];
  uint8_t length[5];
} note_list;

void assigner_notify_on(uint8_t channel, uint8_t note)
{
  if (note_list.length[channel] == NOTE_LIST_MAX)
    return;

  uint8_t i, tmp;
  for (i = 0; i < note_list.length[channel] + 1; i++) {
    if (note < note_list.data[channel][i] || note_list.data[channel][i] == 0) {
      tmp = note_list.data[channel][i];
      note_list.data[channel][i] = note;
      break;
    }
  }

  for (i += 1; i < note_list.length[channel] + 1; i++) {
    uint8_t t = note_list.data[channel][i];
    note_list.data[channel][i] = tmp;
    tmp = t;
  }

  note_list.length[channel]++;
}

void assigner_notify_off(uint8_t channel, uint8_t note)
{
  uint8_t* p = note_list.data[channel];
  for (uint8_t i = 0; i < note_list.length[channel]; i++) {
    if (note_list.data[channel][i] == note)
      i += 1;
    *(p++) = note_list.data[channel][i];
  }
  note_list.length[channel]--;
}

int8_t midi_note_to_note(uint8_t midi_note)
{
  return (int8_t)midi_note - 24;
}

static uint8_t notes[5];

void play_note(uint8_t channel, uint8_t midi_note)
{
  //   notes[channel] = note;

  uint8_t note = midi_note_to_note(midi_note);
  
  switch (channel) {
  case CHN_SQ1:
    env1.gate = 1;
    portamento_target_notes[0] = note;
    break;
	
  case CHN_SQ2:
    env2.gate = 1;
    portamento_target_notes[1] = note;
    break;
	
  case CHN_TRI:
    tri.silenced = 0;
    portamento_target_notes[2] = note;
    break;

  case CHN_NOISE:
    env3.gate = 1;
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
    env1.gate = 0;
    break;
	
  case CHN_SQ2:
    env2.gate = 0;
    break;
	
  case CHN_TRI:
    tri.silenced = 1;
    break;

  case CHN_NOISE:
    env3.gate = 0;
    break;

  case CHN_DMC:
    dmc.sample_enabled = 0;
  }
}

void assigner_handler(void)
{
    
}
