#include <avr/io.h>
#include "memory.h"
#include "bus.h"

/* NESizer

   Memory handling routines.

   Up to 1MB of memory can be selected

 */

inline void set_address(uint32_t address)
/* Sets address in the address latches
   
   mode indicates whether to read or write and should be either MEMORY_READ or MEMORY_WRITE
 */
{
    // Put first 8 address bits in low address latch:
    bus_set_address(MEMORY_ADDRLOW_ADDR);
    bus_set_value(address & 0xFF);
    nop();

    // Shift to next 8 bits
    address >>= 8;
    
    // Put next 8 address bits in mid address latch:
    bus_set_address(MEMORY_ADDRMID_ADDR);
    bus_set_value(address & 0xFF);
    nop();

    // Shift to last 4 bits:
    address >>= 8;

    // Put next 4 address bits in high address latch.
    // The final bit decides between the first and second memory bank.
    // This needs to be translated to corresponding chip select signals.
    bus_set_address(MEMORY_ADDRHIGH_ADDR);
    bus_set_value((address & 0x07) 
		  | ((address & 0x08) ? 0b01000 : 0b10000));
    nop();
}

inline void deselect()
{
    bus_set_address(MEMORY_ADDRHIGH_ADDR);
    bus_set_value(0b11000);
    nop();

    bus_set_address(NO_ADDR);
}

void memory_write(uint32_t address, uint8_t value)
{
    // Set address:
    set_address(address);
    
    // Switch to memory data address and put value on bus:
    bus_set_address(MEMORY_DATA_ADDR);
    we_low();
    bus_set_value(value);
    we_high();

    // Deactivate memory by setting the chip select bits in 
    // high address to zero:
    deselect();
}

uint8_t memory_read(uint32_t address)
{
    // Set address:
    set_address(address);

    bus_set_address(MEMORY_DATA_ADDR);
    bus_set_input();

    uint8_t value = bus_read_value();

    // Switch back to output and deselect memory
    bus_set_output();
    deselect();

    return value;
}

void memory_setup()
{
    // Set WE (pin 5) as output
    DDRC |= WE;

    // Set WE high
    PORTC |= WE;
}
