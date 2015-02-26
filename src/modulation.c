#include <avr/io.h>
#include <avr/pgmspace.h>
#include "modulation.h"
#include "apu.h"
#include "lfo.h"
#include "envelope.h"

uint16_t mod_periods[4] = {0};
uint8_t mod_lfo_modmatrix[4][3] = {{0}};
uint8_t mod_detune[3] = {9, 9, 9};   // detune values default to 9 (translates to 0)
uint8_t mod_envmod[4] = {9, 9, 9, 7};

int8_t dc_temp[3] = {0};

const float neg_cent_table[12] PROGMEM = {
  1, 1.05946f, 1.1225f, 1.1892f, 1.2599f, 1.3348f, 1.4142f, 1.4983f, 1.5874f, 1.6818f, 1.7818f, 1.8877f 
};

const float pos_cent_table[12] PROGMEM = {
  1, 0.9439f, 0.8909f, 0.8409f, 0.7937f, 0.7492f, 0.7071f, 0.6674f, 0.6300f, 0.5946f, 0.5612f, 0.5297f
};

#define ABS(x) ((x > 0) ? (x) : (-x))

static inline int16_t dc_to_dT(uint16_t period, int8_t dc)
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
  float base;

  if (dc == 0)
    return 0;

  uint8_t abs_dc = ABS(dc);
  uint8_t cent_base_index = abs_dc / 10;
  uint8_t cent_offset = abs_dc - cent_base_index * 10;

  if (dc > 0) {
    base = pgm_read_float_near(&pos_cent_table[cent_base_index]);
    return (base * (1.0f - 0.00561f * (float)cent_offset) - 1.0f) * (float)period;
  }
  
  else {
    base = pgm_read_float_near(&neg_cent_table[cent_base_index]);
    return (base * (1.0f + 0.005946f * (float)cent_offset) - 1.0f) * (float)period;
  }
}

//static uint16_t* period_ptrs[] = {&(sq1.period), &(sq2.period), &(tri.period)};
static Envelope* envelopes[] = {&env1, &env2, &env3};

static inline int8_t get_detune(uint8_t chn)
{
    return (int8_t)mod_detune[chn] - 9;
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
	dT = mod_periods[chn] + dc_to_dT(mod_periods[chn], dc_temp[chn]);
	
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
  static LFO* lfos[] = {&lfo1, &lfo2, &lfo3};
  
  int16_t sum = 0;
  uint8_t cnt = 0;

  for (uint8_t j = 0; j < 3; j++) { 
    if (mod_lfo_modmatrix[chn][j] > 0) {
      cnt++;
      //c_sum += mod_lfo_modmatrix[chn][j];
      sum += mod_lfo_modmatrix[chn][j] * lfos[j]->value;
    }
  }
  
  if (cnt > 1) 
    sum /= cnt;
  
  else if (chn <= CHN_TRI) {
    // Frequency delta due to LFO. Divide by 32 to make parameter 30 yield one octave.
    int8_t dc = sum / 32;
    
    // Add detune frequency delta
    dc += get_detune(chn);
    
    // For square channels, also add in the envelope modulation, if any
    int8_t env_fmod_val = (int8_t)envelopes[chn]->value - (int8_t)envelopes[chn]->sustain;
    if (env_fmod_val > 0) 
      dc += 4 * env_fmod_val * get_envmod(chn);
    
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
