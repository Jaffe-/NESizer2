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

const uint8_t mod12[84] PROGMEM = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11  
};

const uint8_t div12[84] PROGMEM = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6
};


int8_t midi_note_to_note(uint8_t midi_note)
/* 
   Takes a note in MIDI note format and returns its corresponding period 
   
   The channel parameter is used to adjust the period for the triangle channel
*/
{
  return (int8_t)midi_note - 24;
}

static uint8_t notes[5];

uint16_t get_period(uint8_t chn, uint16_t c)
/*
  Converts the given frequency change (given in steps of 10 cents) to a 
  corresponding timer value change.

  By direct computation, 
  dT = f_2A03 / (16 * (f + df)) - f_2A03 / (16 * f)
  = f_2A03 / 16 * (f/(f(f + df)) - (f + df) / (f(f + df)))
  = f_2A03 / (16 f) * df / (f + df) 
  = T * (1 - f / (f + df))
  Further more, by the definition of cents and adjusting for dc's 10 cent scale:
  f + df = f * (2^(dc/120) 
  Putting these two together yields
  dT = T * (2^(-dc/120) - 1).

  To simplify calculating the power of 2, a piecewise linear approximation is
  used. 
*/
{
  union {
    uint16_t raw_value;
    struct { 
      uint8_t offset : 4;
      uint16_t semitone : 12;
    };
  } tone;

  tone.raw_value = c;

  uint8_t semitone = pgm_read_byte_near(&mod12[tone.semitone]);
  uint8_t octave = pgm_read_byte_near(&div12[tone.semitone]);
  
  uint16_t period = pgm_read_word_near(&period_table[octave][semitone]);

  // Use linear approximation of 2^x to get to the desired offset
  uint16_t val = (1.0f - 0.00351f * tone.offset) * period;

  if (chn == CHN_TRI)
    val = 0.5f * (val - 1);
  
  // If value is out of bounds, discard the change
  if (val > 2005)
    return 2005;
  else if (val < 8)
    return 8;
  else
    return val;
}


void play_note(uint8_t channel, uint8_t midi_note)
{
  //   notes[channel] = note;

  int16_t note = midi_note_to_note(midi_note);

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
    mod_periods[3] = note - 24;
    break;

  case CHN_DMC:
    if (sample_occupied(note - 60)) {
      sample_load(note - 60);
      if (sample.size != 0)
	dmc.sample_enabled = 1;
      dmc.sample_loop = 0;
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
