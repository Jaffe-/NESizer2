#include "patch.h"
#include "memory.h"
#include "parameter.h"

// First 256 bytes of SRAM not used.
#define PATCH_START 0x100

#define NUM_PARAMS 40
#define PATCH_SIZE 64

void patch_initialize(uint8_t num)
/* Initializes patch memory by writing initial values to the patch memory */
{
    uint16_t address = PATCH_START + PATCH_SIZE * num;

    for (uint8_t i = 0; i < NUM_PARAMS; i++) {
	Parameter data = parameter_get(i);
	memory_write(address++, data.initial_value);
    }
}

void patch_save(uint8_t num)
{
    uint16_t address = PATCH_START + PATCH_SIZE * num;

    for (uint8_t i = 0; i < NUM_PARAMS; i++) {
	Parameter data = parameter_get(i);
	memory_write(address++, *(data.target));
    }
}

void patch_load(uint8_t num)
{
    uint16_t address = PATCH_START + PATCH_SIZE * num;

    for (uint8_t i = 0; i < NUM_PARAMS; i++) {
	Parameter data = parameter_get(i);
	*(data.target) = memory_read(address++);
    }

}
