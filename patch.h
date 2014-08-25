#pragma once

#define PATCH_MIN 0
#define PATCH_MAX 99

const uint16_t PATCH_MEMORY_END;
void patch_save(uint8_t num);
void patch_load(uint8_t num);
void patch_clean();
