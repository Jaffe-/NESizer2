#pragma once

#include "sample.h"

/* The channels */

#define CHN_SQ1 0
#define CHN_SQ2 1
#define CHN_TRI 2
#define CHN_NOISE 3
#define CHN_DMC 4

typedef struct {
    uint8_t enabled;          // BOOL
    uint8_t duty;             // 2-bit integer

    // Used internally:
    uint8_t volume;
    uint16_t period;
} Square;

typedef struct { 
    uint8_t enabled;          // BOOL 
    uint8_t silenced;          // BOOL

    // Used internally:
    uint16_t period;
} Triangle;

typedef struct {
    uint8_t enabled;
    uint8_t loop;             // BOOL: Loop mode (pitched noise) 

    // Used internally:
    uint8_t volume : 4;
    uint8_t period : 4;
} Noise;

typedef struct {
    uint8_t enabled;
    uint8_t sample_loop;       // BOOL: Wether or not sample is automatically looped
    uint8_t sample_number;
    Sample sample;

    // Used internally:
    uint8_t sample_enabled : 1;
    uint8_t data : 7;
} DMC;

Square sq1, sq2;
Triangle tri;
Noise noise;
DMC dmc;

/* Functions */

void sq1_setup();
void sq1_update();
void sq2_setup();
void sq2_update();
void tri_setup();
void tri_update();
void noise_setup();
void noise_update();
void dmc_setup();
void dmc_update();
void dmc_update_sample();
void apu_refresh_channel(uint8_t);
void apu_refresh_all();
void apu_update_handler();
void apu_dmc_update_handler();
