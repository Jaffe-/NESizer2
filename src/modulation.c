#include <avr/io.h>
#include <avr/pgmspace.h>
#include "modulation.h"
#include "apu.h"
#include "lfo.h"
#include "envelope.h"
#include "portamento.h"

#define ABS(x) ((x > 0) ? (x) : (-x))

uint16_t mod_periods[4] = {0};
uint8_t mod_lfo_modmatrix[4][3] = {{0}};
uint8_t mod_detune[3] = {9, 9, 9};   // detune values default to 9 (translates to 0)
uint8_t mod_envmod[4] = {9, 9, 9, 7};
uint16_t mod_pitchbend[4] = {0x2000, 0x2000, 0x2000, 0x2000};

int16_t dc_temp[3] = {0};

const float neg_cent_table[12] PROGMEM = {
  1, 1.05946f, 1.1225f, 1.1892f, 1.2599f, 1.3348f, 1.4142f, 1.4983f, 1.5874f, 1.6818f, 1.7818f, 1.8877f
};

const float pos_cent_table[12] PROGMEM = {
  1, 0.9439f, 0.8909f, 0.8409f, 0.7937f, 0.7492f, 0.7071f, 0.6674f, 0.6300f, 0.5946f, 0.5612f, 0.5297f
};


/* Ugly but necessary for better speed */

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


#define A1_SQ_12 1893
#define A1_SQ_15 1514
#define A1_SQ_16 1419

#define A1_TRI_12 945
#define A1_TRI_15 757
#define A1_TRI_16 709


static inline uint16_t c_to_T(int16_t c)
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
  if (c == 0)
    return A1_SQ_12;

  union {
    uint16_t raw_value;
    struct { 
      uint8_t offset : 4;
      uint16_t semitone : 12;
    };
  } tone;

  tone.raw_value = ABS(c);

  float base;
  uint8_t semitone = pgm_read_byte_near(&mod12[tone.semitone]);
  uint8_t octave = pgm_read_byte_near(&div12[tone.semitone]);
  int16_t val;
  
  if (c > 0) {
    // Get precalculated value for 2^(x/12) where x is the semitone
    base = pgm_read_float_near(&pos_cent_table[semitone]);

    // Use linear approximation to get to the desired offset
    val = (base * (1.0f - 0.00351f * tone.offset)) * A1_SQ_12;

    // If the semitone is above one octave, simply divide by 2
    val >>= octave;
  }
  
  else {
    base = pgm_read_float_near(&neg_cent_table[semitone]);

    val = (base * (1.0f + 0.00372f * tone.offset)) * A1_SQ_12;
    val <<= octave;
  }

  // If value is out of bounds, discard the change
  if (val > 2005)
    return 2005;
  else if (val < 8)
    return 8;
  else
    return val;
}

/*
uint16_t mod_dc_to_T(uint16_t period, int16_t dc)
{
  return c_to_T(period, dc);
}
*/

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
  uint16_t dT;
  if (chn <= CHN_TRI) 
    dT = c_to_T(portamento_cs[chn] + dc_temp[chn]);
	
  switch (chn) {
  case CHN_SQ1:
    sq1.period = dT;
    break;
  case CHN_SQ2:
    sq2.period = dT;
    break;
  case CHN_TRI:
    tri.period = dT;
    break;
  case CHN_NOISE:
    noise.period = mod_periods[CHN_NOISE];
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
//    int16_t dc = portamento_dcs[chn];

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
