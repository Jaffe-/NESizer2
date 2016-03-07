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


#pragma once
#include <stdint.h>
#include <stdbool.h>

uint16_t note_to_period(uint8_t channel, uint8_t note);
void play_note(uint8_t channel, uint8_t note);
void stop_note(uint8_t channel);

void assigner_notify_note_on(uint8_t midi_channel, uint8_t note);
void assigner_notify_note_off(uint8_t midi_channel, uint8_t note);
bool assigner_has_channel(uint8_t midi_channel, uint8_t chn);
void assigner_unset_midi_channel(uint8_t midi_channel, uint8_t chn);
void assigner_set_midi_channel(uint8_t midi_channel, uint8_t chn);

enum assigner_mode {
  MONO,
  POLY1,
  POLY2
};

extern enum assigner_mode assigner_mode;
extern int8_t assigner_midi_channels[5];
