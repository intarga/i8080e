#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "cpu.h"

unsigned char *initalise_memory(char *rom_filename) {
    FILE *f = fopen(rom_filename, "rb");
    if (!f) {
        printf("Could not open %s\n", rom_filename);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *memory = malloc(sizeof(unsigned char) * 16000);

    fread(memory, 1, fsize, f);
    fclose(f);

    return memory;
}

int initalise_state(system_state *state, char *rom_filename) {
    state->pc = 0;
    //TODO more
    state->memory = initalise_memory(rom_filename);
    return 0;
}

int main() {
    system_state state;
    initalise_state(&state, "rom/invaders");

    bool done = false;
    while (!done)
        done = emulate_op(&state);

    return 0;
}
