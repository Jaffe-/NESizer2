#pragma once

// The APU registers
#define PULSE1_CONFIG 0x00
#define PULSE1_SWEEP  0x01
#define PULSE1_TIMER1 0x02
#define PULSE1_TIMER2 0x03

#define PULSE2_CONFIG 0x04
#define PULSE2_SWEEP  0x05
#define PULSE2_TIMER1 0x06
#define PULSE2_TIMER2 0x07

#define TRIANGLE_LENGTH 0x08
#define TRIANGLE_TIMER1 0x0A
#define TRIANGLE_TIMER2 0x0B

#define NOISE_CONFIG 0x0D
#define NOISE_PERIOD 0x0E
#define NOISE_LENGTH 0x0F

#define DMC_CONFIG 0x10
#define DMC_LOAD   0x11
#define DMC_ADDRESS1 0x12
#define DMC_LENGTH 0x13

#define APU_CONTROL 0x15
#define APU_STATUS 0x16
#define APU_FRAME 0x17


/* PULSE channels */

// flags
#define PULSE_LENGTH_CNTR_DISABLE 0b00100000
#define PULSE_LOOP_ENVELOPE 0b00100000
#define PULSE_CONSTANT_VOLUME 0b00010000
#define PULSE_ENVELOPE_DISABLE 0b00010000
#define PULSE_SWEEP_ENABLE 0b10000000
#define PULSE_SWEEP_NEGATIVE 0b00001000

// bit positions
#define PULSE_DUTY_bp 6
#define PULSE_ENVELOPE_PERIOD_bp 0
#define PULSE_VOLUME_bp 0
#define PULSE_LENGTH_CNTR_LOAD_bp 3


/* TRIANGLE channel */

// flags
#define TRIANGLE_LENGTH_CNTR_DISABLE 0b10000000

// bit positions
#define TRIANGLE_LINEAR_RELOAD_bp 0
#define TRIANGLE_LENGTH_CNTR_LOAD_bp 3


/* NOISE channel */

// flags
#define NOISE_LOOP_ENVELOPE 0b00100000
#define NOISE_LENGTH_CNTR_DISABLE 0b00100000
#define NOISE_CONSTANT_VOLUME 0b00010000
#define NOISE_ENVELOPE_DISABLE 0b00010000
#define NOISE_LOOP_ENABLE 0b10000000

// bit positions
#define NOISE_LENGTH_CNTR_LOAD_bp 3


/* DMC channel */

// flags
#define DMC_IRQ_ENABLE 0b10000000
#define DMC_LOOP_SAMPLE 0b01000000


/* CONTROL register */

#define CONTROL_DMC_ENABLE 0b00010000
#define CONTROL_LENGTH_NOISE 0b00001000
#define CONTROL_LENGTH_TRIANGLE 0b00000100
#define CONTROL_LENGTH_PULSE2 0b00000010
#define CONTROL_LENGTH_PULSE1 0b00000001


/* FRAME register */

#define FRAME_5STAGE 0b10000000
#define FRAME_INTERRUPT_DISABLE 0b01000000


// The 6502 opcodes needed
#define LDA_imm 0xA9
#define STA_abs 0x8D
#define NOP 0xEA
#define LDA_abs 0xAD

// Pins used to interface with 6502
#define RES 1
#define PHI2 0b10

/* 
   The following macro waits for PHI2 to transition from
   low to high, signalling that the 6502 is ready to
   receive data on the databus.
*/
#define databus_wait() do {       \
	if(!(PORTC & PHI2))       \
            while(PORTC & PHI2);  \
        while(!(PORTC & PHI2));	  \
	} while(0)

#define databus_set(_VAL) do {     \
        databus_wait();           \
        PORTD = _VAL;              \
        } while(0)

void register_write(uint8_t reg, uint8_t val);
void register_write_final(uint8_t reg, uint8_t val);
uint8_t status_read();
void setup_2a03();

