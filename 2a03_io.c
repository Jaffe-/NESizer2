#include "constants.h"
#include <avr/io.h>
#include <util/delay.h>
#include "2a03_io.h"

/* 
   NES APU interface

   The APU is located on-die in the 2A03 IC. It is connected
   internally to the 6502 CPU, and there are no external pins 
   facilitating communication with the APU directly. This means
   that all control of the APU must be done by using the 6502 to
   store values in the APU registers. 

   PORTD is connected to the 6502 databus. 

*/

void register_write(uint8_t reg, uint8_t val)
/* Internal function for writing to register
  
   Writes a value to an APU register by feeding the 6502
   with instructions for loading the value into A, and then
   storing the value in $4014.
 */
{
    // Wait for 6502's read and but LDA_imm on databus
    databus_set(LDA_imm);
    databus_set(val);

    databus_set(STA_abs);
    databus_set(reg);
    databus_set(0x40);
}

void register_write_final(uint8_t reg, uint8_t val)
/* Write a value to a register
   
   This function writes a value to a register
 */
{
    register_write(reg, val);

    // Have to put NOP on the buss to not halt the 6502
    databus_set(NOP);
}

uint8_t read_status()
/* Read status register 

   This function reads the status register in the APU by making the CPU
   load the register into A and capturing the value on the databus when
   it does the read. 
*/
{
    // Make the CPU read from status register (0x4015 in its address space)
    databus_set(LDA_abs);
    databus_set(0x15);
    databus_set(0x40);

    // Wait for PHI2 to go high
    databus_wait();

    // Set all PORT D pins as input and tri-state them. This is done after PHI2 goes
    // low to waste a few cycles, to be sure that the value the 6502 reads from the
    // APU register has appeared on the bus. 
    PORTD = 0;
    DDRD = 0;

    // Capture the value on the databus
    uint8_t val = PIND;
    
    // Configure PORT D as output again and send a NOP
    DDRD = 1;
    PORTD = 0;     
   
    PORTD = NOP;

    return val;
}

void reset_2a03()
/* Simply resets the 6502. */
{
    // Set /RES low
    PORTC &= ~RES;

    // Hold it low for some time, for 6502 to perform reset
    _delay_ms(1);

    // Set /RES high again
    PORTC |= RES;
}

void setup_2a03()
/* Initializes the interface to communicate with 6502 

   Configures ports and resets the 6502 and puts the APU in a suitable
   (non-interruptive) state.
*/
{
    // Configure all of PORTD as output and set value to NOP opcode
    DDRD = 0xFF;
    PORTD = NOP;

    // Configure the /RES pin on PORT C as output and set /RES high
    DDRC |= RES;
    PORTC |= RES;

    // Reset the 2A03
    reset_2a03();

    // Now the 6502 should be ready to receive instructions (?)

    // We need to disable the frame interrupt
    register_write(APU_FRAME, FRAME_INTERRUPT_DISABLE);

    // Ensure that DMC channel does not trigger IRQ
    register_write(DMC_CONFIG, 0);
}

