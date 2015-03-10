/*
  NESIZER 

  Note assigner

  Assigns notes to channels
*/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "modulation.h"
#include "portamento.h"
#include "apu.h"
//#include "lfo.h"
#include "envelope.h"
#include "assigner.h"
#include "sample.h"
#include "periods.h"

int8_t midi_note_to_note(uint8_t midi_note)
/* 
   Takes a note in MIDI note format and returns its corresponding period 
   
   The channel parameter is used to adjust the period for the triangle channel
*/
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
    if (get_envmod(CHN_TRI) != 0)
      env3.gate = 1;
    portamento_target_notes[2] = note;
    break;

  case CHN_NOISE:
    env3.gate = 1;
    noise_period = note - 24;
    break;

  case CHN_DMC:
    if (sample_occupied(midi_note - 60)) {
      sample_load(midi_note - 60);
      if (sample.size != 0)
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

void assigner_handler()
{
    
}
