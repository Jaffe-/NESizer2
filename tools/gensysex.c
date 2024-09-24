#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "../src/midi/sysex.h"

int main(int argc, char **argv)
{
    if (argc < 4) {
        printf("gensysex type index in-file out-file\n");
        return 1;
    }
    const char *type = argv[1];
    const char *index_s = argv[2];
    const char *infile = argv[3];
    const char *outfile = argv[4];

    const unsigned int index_i = atoi(index_s);

    if (index_i > 255) {
        printf("file-number must be 0-255\n");
        return 1;
    }
    uint8_t index = index_i;

    FILE *in, *out;

    in = fopen(infile, "rb");
    if (!in) {
        printf("Failed to open input file %s: %s\n", infile, strerror(errno));
        return 1;
    }
    out = fopen(outfile, "wb");
    if (!out) {
        printf("Failed to open output file %s: %s\n", outfile, strerror(errno));
        fclose(in);
        return 1;
    }

    fseek(in, 0, SEEK_END);
    size_t size_in = ftell(in);
    fseek(in, 0, SEEK_SET);

    size_t size_header = 1; // always at least a command byte

    enum sysex_data_format data_format;
    enum sysex_cmd sysex_cmd;
    uint8_t header[10];

    if (!strcmp(type, "sample")) {
        sysex_cmd = SYSEX_CMD_SAMPLE_LOAD;
        data_format = SYSEX_DATA_FORMAT_7BIT_TRUNC;
        /* 1 byte for index
           1 byte for sample type
           3 bytes for size */
        size_header += 5;
        header[1] = index;
        header[2] = 0; // raw PCM
        header[3] = size_in & 0x7F;
        header[4] = (size_in >> 7) & 0x7F;
        header[5] = (size_in >> 14) & 0x7F;
    }
    else if (!strcmp(type, "settings")) {
        sysex_cmd = SYSEX_CMD_SETTINGS_LOAD;
        data_format = SYSEX_DATA_FORMAT_4BIT;
        printf("settings: index argument is ignored");
    }
    else if (!strcmp(type, "patch")) {
        sysex_cmd = SYSEX_CMD_PATCH_LOAD;
        data_format = SYSEX_DATA_FORMAT_4BIT;
        if (index > 99) {
            printf("patch: index must be in range 0 .. 99");
            return 1;
        }
        /* 1 byte for index */
        size_header += 1;
        header[1] = index;
    }
    else if (!strcmp(type, "sequence")) {
        sysex_cmd = SYSEX_CMD_SEQUENCE_LOAD;
        data_format = SYSEX_DATA_FORMAT_4BIT;
        if (index > 99) {
            printf("sequence: index must be in range 0 .. 99");
            return 1;
        }
        /* 1 byte for index */
        size_header += 1;
        header[1] = index;
    }
    else {
        printf("unsupported file type %s\n", type);
    }

    header[0] = sysex_cmd;

    size_t size_total = size_header + 4;  // +1 byte each for System Exclusive, SysexId, DeviceId, and End of Exclusive
    switch (data_format) {
    default:
    case SYSEX_DATA_FORMAT_4BIT:
        size_total += size_in * 2;
        break;
    case SYSEX_DATA_FORMAT_7BIT_TRUNC:
        size_total += size_in;
        break;
    }

    char *buf = calloc(size_total, 1);

    /* System Exclusive */
    buf[0] = 0xF0;
    buf[1] = SYSEX_ID;
    buf[2] = SYSEX_DEVICE_ID;

    for (size_t i = 0; i < size_header; i ++)
        buf[i + 3] = header[i];

    /* Pack data according to given packing format */
    for (size_t fi = 0; fi < size_in; fi++) {
        size_t i = 3 + size_header + fi;
        uint8_t val;
        fread(&val, 1, 1, in);
        switch (data_format) {
        default:
        case SYSEX_DATA_FORMAT_4BIT:
            buf[2*i] = (val >> 4) & 0x0F;
            buf[2*i + 1] = val & 0x0F;
            break;
        case SYSEX_DATA_FORMAT_7BIT_TRUNC:
            buf[i] = (val >> 1) & 0x7F;
            break;
        }
    }

    /* End of Exclusive */
    buf[size_total - 1] = 0xF7;

    fwrite(buf, size_total, 1, out);
    fclose(in);
    fclose(out);
}
