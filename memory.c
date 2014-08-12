#include <avr/io.h>
#include "memory.h"
#include "bus.h"

inline void set_address(uint16_t address)
/* Sets address in the address latches
   
   mode indicates whether to read or write and should be either MEMORY_READ or MEMORY_WRITE
 */
{
    // Put low part of address in the low address latch:
    bus_set_address(MEMORY_ADDRLOW_ADDR);
    bus_set_value(address & 0xFF);
    
    nop();

    // Put high part (upper 6 bits) in high address latch. The upper 2 bits indicate whether to read or write.
    bus_set_address(MEMORY_ADDRHIGH_ADDR);
    bus_set_value((address >> 8) & 0x3F);

    nop();

    bus_set_address(NO_ADDR);
}

void memory_write(uint16_t address, uint8_t value)
{
    // Set address:
    set_address(address);
    
    // Switch to memory data address and put value on bus:
    bus_set_address(MEMORY_DATA_WRITE_ADDR);
    bus_set_value(value);

    // Deselect memory
    bus_set_address(NO_ADDR);
}

uint8_t memory_read(uint16_t address)
{
    // Set address:
    set_address(address);

    bus_set_address(MEMORY_DATA_READ_ADDR);
    bus_set_input();

    uint8_t value = bus_read_value();

    // Switch back to output and deselect memory

    bus_set_address(NO_ADDR);
    bus_set_output();

    return value;
}
