/*
  Copyright 2024 Johan Fjeldtvedt and Beau Sterling

  This file is part of NESIZER.

  NESIZER is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  NESIZER is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NESIZER.  If not, see <http://www.gnu.org/licenses/>.



  SysEx

  Parses incoming SysEx messages and loads samples, changes patches
  or ignores them with utmost disinterest
*/


#pragma once

#include <stdint.h>

// Used for ignoring unwanted sysex messages:
#define SYSEX_ID 0x7D         // 7D is the "Special ID", reserved for non-commerical use
#define SYSEX_DEVICE_ID 0x4E  // 4E is "N" in ASCII hex, for NESizer :)

#define SAMPLE_SIZE_BYTES 3

enum sysex_cmd {
    SYSEX_CMD_SAMPLE_LOAD = 1,
    SYSEX_CMD_SETTINGS_LOAD,
    SYSEX_CMD_PATCH_LOAD,
    SYSEX_CMD_SEQUENCE_LOAD,
};

enum sysex_data_format {
    SYSEX_DATA_FORMAT_4BIT,
    SYSEX_DATA_FORMAT_7BIT_TRUNC,
};

struct sysex_header {
    uint8_t sysex_id;
    uint8_t device_id;
    uint8_t command;

    uint8_t sample_number;
    uint8_t sample_type;
    uint32_t sample_size;
    uint8_t sample_size_bytes;

    uint8_t data_ready;
};

extern uint8_t midi_transfer_progress;

void sysex(void);
void transfer(void);
void ignore_sysex(void);
