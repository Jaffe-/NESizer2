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


#include "sysex.h"
#include "midi.h"
#include "io/midi.h"
#include "apu/apu.h"
#include "assigner/assigner.h"
#include "patch/patch.h"
#include "sample/sample.h"
#include "settings/settings.h"
#include "ui/ui.h"
#include "ui/ui_programmer.h"


#include "softserial_debug/debug.h"


#define SYSEX_STOP 0xF7
#define MIDI_STATUS_UNDEF 0xFD

static inline void initiate_transfer(void);

uint8_t midi_transfer_progress = 0;

static struct sample sample;

extern enum midi_state state;

struct sysex_header syx_header = {
    .sysex_id = MIDI_STATUS_UNDEF,
    .device_id = MIDI_STATUS_UNDEF,
    .command = MIDI_STATUS_UNDEF,
    .data_ready = 0,
    .sample_number = MIDI_STATUS_UNDEF,
    .sample_type = MIDI_STATUS_UNDEF,
    .sample_size = 0,
    .sample_size_bytes = 0
};

void reset_sysex_header(struct sysex_header *hdr)
{
    state = STATE_MESSAGE;
    hdr->sysex_id = MIDI_STATUS_UNDEF;
    hdr->device_id = MIDI_STATUS_UNDEF;
    hdr->command = MIDI_STATUS_UNDEF;
    hdr->data_ready = 0;
    hdr->sample_number = MIDI_STATUS_UNDEF;
    hdr->sample_type = MIDI_STATUS_UNDEF;
    hdr->sample_size = 0;
    hdr->sample_size_bytes = 0;
}

uint8_t valid_header_byte(uint8_t hdr_byte)
{
    if (hdr_byte != MIDI_STATUS_UNDEF)
        return 1;
    else
        return 0;
}

uint8_t sysex_stop_byte(uint8_t syx)
{
    if (syx != SYSEX_STOP) {
        return 0;
    } else {
        reset_sysex_header(&syx_header);
        return 1;
    }
}


void sysex()
{
    uint8_t val;
    if (midi_io_bytes_remaining() > 0) {
        if (!syx_header.data_ready) val = midi_io_read_byte();
        if (sysex_stop_byte(val)) return;  // check for 0xF7

        // When the state just changed, we need to look at the first few bytes
        // to determine what sysex message we're dealing with
        if (!valid_header_byte(syx_header.command)) {
            if(!valid_header_byte(syx_header.sysex_id)) {
                syx_header.sysex_id = val;
            }

            else if (!valid_header_byte(syx_header.device_id)) {
                syx_header.device_id = val;

                if (!(syx_header.sysex_id == SYSEX_ID) ||
                    !(syx_header.device_id == SYSEX_DEVICE_ID)) {  // ignore messages not meant for NESizer
                    ignore_sysex();
                }
            }

            else { syx_header.command = val; }
        }

        else {
            if (syx_header.command == SYSEX_CMD_SAMPLE_LOAD) {
                /*
                    example message (loads sample to slot 2):
                    F0    7D    4E    01    1C    00    2B    19    00    ....    F7
                    STRT  {  ID  }    CMD   SLOT  TYPE  { SAMPLESIZE }    DATA    END
                */
                if (!syx_header.data_ready) {
                    // Read sample descriptor and store in sample object
                    if(!valid_header_byte(syx_header.sample_number))
                        syx_header.sample_number = val;

                    else if (!valid_header_byte(syx_header.sample_type))
                        syx_header.sample_type = val;

                    else if(syx_header.sample_size_bytes < SAMPLE_SIZE_BYTES - 2) {
                        syx_header.sample_size = val;
                        ++syx_header.sample_size_bytes;
                    }
                    else if(syx_header.sample_size_bytes < SAMPLE_SIZE_BYTES - 1) {
                        syx_header.sample_size |= (uint32_t)val << 7;
                        ++syx_header.sample_size_bytes;
                    }
                    else if(syx_header.sample_size_bytes < SAMPLE_SIZE_BYTES) {
                        syx_header.sample_size |= (uint32_t)val << 14;
                        ++syx_header.sample_size_bytes;
                        syx_header.data_ready = 1;
                    }
                }

                else {
                    sample.type = syx_header.sample_type;
                    sample.size = syx_header.sample_size;
                    sample_new(&sample, syx_header.sample_number);
                    initiate_transfer();
                }
            }

            else if (syx_header.command == SYSEX_CMD_PATCH_LOAD) {
                /*
                    example message (selects patch # 4):
                    F0    7D    4E    03    04    F7
                    STRT  {  ID  }    CMD   ##    END
                */
                uint8_t patch_byte = val;
                if (patch_pc_limit(get_patchno_addr(), PATCH_MIN, PATCH_MAX, patch_byte)) {  // range limit
                    patch_load(patch_byte);
                    settings_write(PROGRAMMER_SELECTED_PATCH, patch_byte);
                }
                ignore_sysex();  // ignore any extra bytes
            }

            else {
                ignore_sysex();
            }
        }
    }
}

#define ERROR_MIDI_RX_LEN_MISMATCH (1 << 2)

void transfer()
/*
  Handles transfering of data via MIDI
*/
{
    uint8_t val;
    while (midi_io_bytes_remaining() > 0) {
        val = midi_io_read_byte();

        if (val == SYSEX_STOP) {
            ui_pop_mode();
            reset_sysex_header(&syx_header);
            if (sample.bytes_done != sample.size)
                error_set(ERROR_MIDI_RX_LEN_MISMATCH);
        }

        else if ((val & 0x80) == 0) {
            if (sample.bytes_done < sample.size) {
                sample_write_serial(&sample, val);
                midi_transfer_progress = (sample.bytes_done << 4) / sample.size;
            }
        }
    }
}

static inline void initiate_transfer()
{
    // Set UI mode to transfer (which turns the button LEDs into a status bar
    // for the duration of the transfer)
    ui_push_mode(MODE_TRANSFER);

    // Disable DMC
    dmc.sample_enabled = 0;

    state = STATE_TRANSFER;
    midi_transfer_progress = 0;
}

void ignore_sysex()
{
    // for handling streams of sysex data not meant for NESizer.
    state = STATE_IGNORE_SYSEX;

    leds_7seg_custom(3, 0b00101010);  // n
    leds_7seg_custom(4, 0b00111010);  // o

    while (midi_io_bytes_remaining() >= 1) {
        if (sysex_stop_byte(midi_io_read_byte()))
            return;
    }
}
