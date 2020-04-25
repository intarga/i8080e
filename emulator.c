#include <stdio.h>
#include <stdlib.h>
#include "disassembler.h"

unsigned char *read_rom(char *filename, int *fsize) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("Could not open %s\n", filename);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    *fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *buffer = malloc(*fsize);

    fread(buffer, 1, *fsize, f);
    fclose(f);

    return buffer;
}

int main() {
    int *fsize;
    unsigned char *buffer = read_rom("rom/invaders.h", fsize);

    int pc = 0;
    while (pc < *fsize) {
        pc += disassemble_op(buffer, pc);
    }
}
