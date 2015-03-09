#include <avr/io.h>
#include <avr/pgmspace.h>
#include "modulation.h"
#include "apu.h"
#include "lfo.h"
#include "envelope.h"
#include "portamento.h"

#define ABS(x) ((x > 0) ? (x) : (-x))

uint8_t mod_lfo_modmatrix[4][3];
uint8_t mod_detune[3] = {9, 9, 9};   // detune values default to 9 (translates to 0)
uint8_t mod_envmod[4] = {9, 9, 9, 7};
uint16_t mod_pitchbend[4] = {0x2000, 0x2000, 0x2000, 0x2000};

static int16_t dc_temp[3];
uint8_t noise_period;

static Envelope* envelopes[] = {&env1, &env2, &env3};

static inline int8_t get_detune(uint8_t chn)
{
  return (int8_t)mod_detune[chn] - 9;
}

static inline int16_t get_pitchbend(uint8_t chn)
{
  return (int16_t)(mod_pitchbend[chn] >> 5) - 0x100;
}

int8_t get_envmod(uint8_t chn)
{
  if (chn == CHN_NOISE)
    return (int8_t)mod_envmod[2] - 7;
  else 
    return (int8_t)mod_envmod[chn] - 9;
}

static inline void apply_freqmod(uint8_t chn)
/*
  SQ1/2/TRI: Applies calculated frequency modulations by converting them to 
  period compensated period modulations. 

  NOISE: Applies envelope modulation and LFOs directly to period
*/
{
  // Convert frequency delta to a period delta and add to the base period
  uint16_t period = 0;
  if (chn <= CHN_TRI) 
    period = get_period(chn, portamento_cs[chn] + dc_temp[chn]);
	
  switch (chn) {
  case CHN_SQ1:
    sq1.period = period;
    break;
  case CHN_SQ2:
    sq2.period = period;
    break;
  case CHN_TRI:
    tri.period = period;
    break;
  case CHN_NOISE:
    noise.period = noise_period;
  }
}

static inline void calc_freqmod(uint8_t chn)
/*
  Calculates frequency change for SQ1, SQ2 and TRI based on 
  detuning, LFOs and envelope modulation.
*/
{
  // Define some helper arrays
  static const LFO* lfos[] = {&lfo1, &lfo2, &lfo3};
  
  int16_t sum = 0;

  for (uint8_t j = 0; j < 3; j++) { 
    if (mod_lfo_modmatrix[chn][j] > 0) 
      sum += mod_lfo_modmatrix[chn][j] * lfos[j]->value;
  }
    
  if (chn <= CHN_TRI) {
    // Frequency delta due to LFO. Divide by 32 to make parameter 30 yield one octave.
    int16_t dc = 0;
    
    dc += get_pitchbend(chn);

    dc += sum / 128;
    
    // Add detune frequency delta
    dc += get_detune(chn);
    
    // For square channels, also add in the envelope modulation, if any
    int8_t env_fmod_val = (int8_t)envelopes[chn]->value - (int8_t)envelopes[chn]->sustain;
    if (env_fmod_val > 0) 
      dc += env_fmod_val * get_envmod(chn);
    
    // Store total dc value, which will be applied by apply_freqmod
    dc_temp[chn] = dc;
    
  }
}

static inline void apply_envelopes()
{
  sq1.volume = env1.value;
  sq2.volume = env2.value;
  noise.volume = env3.value;    
}

void mod_calculate()
{
  static uint8_t chn = 0;
  calc_freqmod(chn); 
  if (++chn == 4) chn = 0;
}

void mod_apply()
{
  static uint8_t chn = 0;
  apply_freqmod(chn); 
  if (++chn == 4) chn = 0;

  apply_envelopes(); 
}
