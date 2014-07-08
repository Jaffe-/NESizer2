#pragma once

/* The channels */

#define CHN_SQ1 0
#define CHN_SQ2 1
#define CHN_TRI 2
#define CHN_NOISE 3
#define CHN_DMC 4

#define SQ1_bm 1
#define SQ2_bm 0b10
#define TRI_bm 0b100
#define NOISE_bm 0b1000
#define DMC_bm 0b10000


/* The APU registers */

#define SQ1_VOL 0x00
#define SQ1_SWEEP  0x01
#define SQ1_LO 0x02
#define SQ1_HI 0x03

#define SQ2_VOL 0x04
#define SQ2_SWEEP 0x05
#define SQ2_LO 0x06
#define SQ2_HI 0x07

#define TRI_LINEAR 0x08
#define TRI_LO 0x0A
#define TRI_HI 0x0B

#define NOISE_VOL 0x0C
#define NOISE_LO 0x0E
#define NOISE_HI 0x0F

#define DMC_FREQ 0x10
#define DMC_RAW   0x11
#define DMC_START 0x12
#define DMC_LEN 0x13

#define SND_CHN 0x15


/* Common bit masks and positions */

#define VOLUME_m 0b00001111
#define PERIOD_HI_m 0b00000111

#define VOLUME_p 0
#define PERIOD_HI_p 0

#define LENGTH_CNTR_LOAD_p 3


/* Square channels */

// flags
#define SQ_LENGTH_CNTR_DISABLE 0b00100000
#define SQ_CONSTANT_VOLUME 0b00010000

// bit positions
#define SQ_DUTY_p 6
#define SQ_VOLUME_p 0
#define SQ_LENGTH_CNTR_LOAD_p 3

// masks
#define SQ_DUTY_m 0b11000000


/* Triangle channel */

// flags
#define TRI_LENGTH_CNTR_DISABLE 0b10000000

// bit positions
#define TRI_LINEAR_RELOAD_bp 0
#define TRI_LENGTH_CNTR_LOAD_bp 3


/* Noise channel */

// flags
#define NOISE_LENGTH_CNTR_DISABLE 0b00100000
#define NOISE_CONSTANT_VOLUME 0b00010000

// bit positions
#define NOISE_LENGTH_CNTR_LOAD_bp 3
#define NOISE_LOOP_p 7
#define NOISE_PERIOD_p 0
#define NOISE_HW_ENV_p 4

// masks
#define NOISE_LOOP_m 0b10000000
#define NOISE_PERIOD_m 0b00001111
#define NOISE_HW_ENV_m 0b00010000

/* DMC channel */

// flags
#define DMC_IRQ_ENABLE 0b10000000
#define DMC_LOOP_SAMPLE 0b01000000


/* Control register */

#define DMC_ENABLE_m 0b00010000
#define NOISE_ENABLE_m 0b00001000
#define TRI_ENABLE_m 0b00000100
#define SQ2_ENABLE_m 0b00000010
#define SQ1_ENABLE_m 0b00000001

#define DMC_ENABLE_p 4
#define NOISE_ENABLE_p 3
#define TRI_ENABLE_p 2
#define SQ2_ENABLE_p 1
#define SQ1_ENABLE_p 0

typedef struct {
    uint8_t duty : 2;
    uint8_t volume;
    uint8_t enabled : 1;

    // Used internally:
    uint16_t period;
} Square;

typedef struct { 
    uint8_t enabled : 1;

    // Used internally:
    uint16_t period;
} Triangle;

typedef struct {
    uint8_t volume;
    uint8_t loop : 1;
    uint8_t enabled : 1;
    uint8_t hw_env : 1;

    // Used internally:
    uint8_t period : 4;
} Noise;

typedef struct {
    uint8_t data : 7;
    uint8_t enabled : 1;
    uint8_t sample_enabled : 1;
    uint8_t sample_loop : 1;
    uint8_t* sample;
    uint16_t sample_length;

    // Used internally:
    uint16_t current;
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
void dmc_update_sample_dpcm();
void dmc_update_sample_raw();
void apu_refresh_channel(uint8_t);
void apu_refresh_all();
