#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "deltacompress.h"

int compress(unsigned char* data, int size, unsigned char* output)
{
    // Initial start value is the same
    output[0] = data[0];

    int out_ptr = 1;

    unsigned char accumulator = output[0];

    for (int i = 0; i < size - 1; i++) {
	char diff = data[i + 1] - accumulator;

	unsigned char value;
	if (diff >= decision_table[15])
	    value = 15;
	else {
	    for (value = 0; value < 15; value++) {
		if (diff <= decision_table[value])
		    break;
	    }
	}

	accumulator += delta_table[value];

	printf("%i -> %i\t", diff, delta_table[value]);
	if (i >= 10 && i % 10 == 0) printf("\n");


	if (i % 2 == 0) 
	    output[out_ptr] = value;
	else {
	    output[out_ptr] |= value << 4; 
	    out_ptr++;
	}
    }
    
    return out_ptr;
}

int decompress(unsigned char* data, int size, unsigned char* output) 
{
    int ptr_out = 0;
    unsigned char accumulator = data[0];
    output[ptr_out] = accumulator;

    for (int i = 1; i < size; i++) {
	char delta = delta_table[data[i] & 0x0F];
	accumulator += delta;
	output[++ptr_out] = accumulator;

	delta = delta_table[(data[i] >> 4) & 0x0F];
	accumulator += delta;
	output[++ptr_out] = accumulator;
    }
    return ptr_out;
}

int main(int argc, char** argv)
{
    if (argc < 3) {
	puts("dc [-c COMPRESS -d DECOMPRESS] input output\n");
	return(1);
    }

    FILE *in, *out, *diff;
    
    in = fopen(argv[2], "rb");
    out = fopen(argv[3], "wb");

    fseek(in, 0, SEEK_END);
    int size_in = ftell(in);
    fseek(in, 0, SEEK_SET);

    int size_out = (!strcmp(argv[1], "-c")) ? ceil(size_in / 2.0) + 1 : (size_in - 1) * 2;

    unsigned char* data = (unsigned char*)malloc(sizeof(unsigned char) * size_in);
    unsigned char* output = (unsigned char*)malloc(sizeof(unsigned char) * size_out);

    fread(data, 1, size_in, in);

    int v = (!strcmp(argv[1], "-c")) ? compress(data, size_in, output) : decompress(data, size_in, output);

    fwrite(output, 1, size_out, out);

    printf("Read %i bytes from file %s and wrote %i bytes to file %s\n", size_in, argv[2], size_out, argv[3]);
    printf("%i\n", v);

    fclose(in);
    fclose(out);
}
