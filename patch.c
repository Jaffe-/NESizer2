#include "patch.h"
#include "memory.h"
#include "parameter.h"

#define PATCH_OFFSET 0

#define BYTE 0
#define WORD 1
#define DWORD 2

#define NUM_PARAMS 40
#define PATCH_SIZE 64

void patch_initialize(uint8_t num)
/* Initializes patch memory by writing initial values to the patches */
{
    uint16_t address = PATCH_SIZE * num;

    for (uint8_t i = 0; i < NUM_PARAMS; i++) {
	Parameter data = parameter_get(i);
	memory_write(address++, data.initial_value);
    }
}

void patch_save(uint8_t num)
{
    uint16_t address = PATCH_SIZE * num;

    for (uint8_t i = 0; i < NUM_PARAMS; i++) {
	Parameter data = parameter_get(i);
	memory_write(address++, *(data.target));
    }
}

void patch_load(uint8_t num)
{
    uint16_t address = PATCH_SIZE * num;

    for (uint8_t i = 0; i < NUM_PARAMS; i++) {
	Parameter data = parameter_get(i);
	*(data.target) = memory_read(address++);
    }

}
