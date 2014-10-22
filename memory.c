#include <avr/io.h>
#include "memory.h"
#include "bus.h"

/* 
   NESizer

   External memory routines.

*/

// Current values in the address latches.
// Reduces the need for costly 32 bit calculations
// when writing sequentially
uint8_t current_low;
uint8_t current_mid;
uint8_t current_high;

inline void set_addrlow(uint8_t addrlow)
{
    bus_set_address(MEMORY_ADDRLOW_ADDR);
    bus_set_value(addrlow);
}

inline void set_addrmid(uint8_t addrmid)
{
    bus_set_address(MEMORY_ADDRMID_ADDR);
    bus_set_value(addrmid);
}

inline void set_addrhigh(uint8_t addrhigh)
{
    bus_set_address(MEMORY_ADDRHIGH_ADDR);
    bus_set_value(addrhigh | ((addrhigh & 0x08) ? 0b01000 : 0b10000));
}

inline void deselect()
/*
  Sets both chip select bits to zero in the high latch.
*/
{
    bus_set_address(MEMORY_ADDRHIGH_ADDR);
    bus_set_value(current_high | 0b11000);
    nop();

    bus_set_address(NO_ADDR);
}

static uint8_t read_data()
{
    bus_set_address(MEMORY_DATA_ADDR);
    bus_set_input();

    uint8_t value = bus_read_value();

    bus_set_output();

    return value;
}

static void write_data(uint8_t value)
{
    // Switch to memory data address and put value on bus:
    bus_set_address(MEMORY_DATA_ADDR);
    we_low();
    bus_set_value(value);
    we_high();
}

static void inc_address()
{
    set_addrlow(++current_low);
    if (current_low == 0)
	set_addrmid(++current_mid);

    if (current_mid == 0)
	// Current high is increased, but not written to the latch until
	// the SRAM is about to be used (the chip select bits have to be set).
	++current_high;

}

void memory_set_address(uint32_t address)
/* 
   Sets an adress. The latch values are saved for
   sequential writing.
 */
{

    // Put first 8 address bits in low address latch:
    set_addrlow(current_low = address & 0xFF);
    nop();

    // Shift to next 8 bits
    address >>= 8;
    
    // Put next 8 address bits in mid address latch:
    set_addrmid(current_mid = address & 0xFF);
    nop();

    // Shift to last 4 bits:
    address >>= 8;

    // Put next 4 address bits in high address latch.
    // The final bit decides between the first and second memory bank.
    // This needs to be translated to corresponding chip select signals.
    set_addrhigh(current_high = address & 0x07);
    nop();
}


void memory_write(uint32_t address, uint8_t value)
{
    memory_set_address(address);

    write_data(value);

   // Deactivate memory by setting the chip select bits in 
    // high address to zero:
    deselect();
}

uint8_t memory_read(uint32_t address)
{
    // Set address:
    memory_set_address(address);

    uint8_t value = read_data();

    // Deactivate memory
    deselect();
	
    return value;
}

uint8_t memory_read_sequential()
/*
  Reads a byte at the current address and increments address by 1.
 */
{
    // Set high byte. This has to be done even though the high byte 
    // might not have (and probably hasn't) changed, because the chip 
    // select bit has to be set. 
    set_addrhigh(current_high);

    uint8_t value = read_data();

    deselect();

    inc_address();

    return value;
}

void memory_write_sequential(uint8_t value)
{
    set_addrhigh(current_high);

    write_data(value);

    deselect();

    inc_address();
}

void memory_write_word(uint32_t address, uint16_t value)
{
    memory_set_address(address);
    memory_write_sequential(value & 0xFF);
    memory_write_sequential((value >> 8) & 0xFF);
}

uint16_t memory_read_word(uint32_t address)
{
    memory_set_address(address);
    uint16_t value = memory_read_sequential();
    value |= (uint16_t)memory_read_sequential();
    return value;
}

void memory_write_dword(uint32_t address, uint32_t value)
{
    memory_set_address(address);
    memory_write_sequential(value & 0xFF);
    memory_write_sequential((value >> 8) & 0xFF);
    memory_write_sequential((value >> 16) & 0xFF);
    memory_write_sequential((value >> 24) & 0xFF);
}

uint32_t memory_read_dword(uint32_t address)
{
    memory_set_address(address);
    uint32_t value = memory_read_sequential();
    value |= (uint32_t)memory_read_sequential() << 8;
    value |= (uint32_t)memory_read_sequential() << 16;
    value |= (uint32_t)memory_read_sequential() << 24;
    return value;
}

void memory_clean()
{
    for (uint32_t i = 0; i < MEMORY_SIZE; i++) 
	memory_write(i, 0);
}

void memory_setup()
{
    // Set WE (pin 5) as output
    DDRC |= WE;

    // Set MEM_EN pin as output
    DDRB |= MEM_EN;

    // Set WE high
    PORTC |= WE;

    // Make sure the upper address latch doesn't enable its outputs
    deselect();
}

