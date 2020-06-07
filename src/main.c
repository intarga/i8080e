#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "cpu.h"
#include "display.h"
#include "input.h"

#define FRAMERATE 60
#define CYCLES_PER_FRAME 2000000 / FRAMERATE

typedef struct {
    uint16_t shift;
    uint8_t offset;
} Port;

typedef struct {
    Cpu_state *state;
    Display *display;
    Input *input;
    Port *port;
} Arcade_system;

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
    memset(memory, 0, 16000);

#if CPUDIAG //TODO implement without #if
    fread(&memory[0x100], 1, fsize, f);
#else
    fread(memory, 1, fsize, f);
#endif
    fclose(f);

    return memory;
}

int initalise_state(Cpu_state *state, char *rom_filename) {
    state->pc = 0;
    state->sp = 0;
    state->int_enable = 0;
    state->cc.ac = 0;
    state->cc.cy = 0;
    state->cc.z = 0;
    state->cc.s = 0;
    state->cc.p = 0;
    for (int i = 0; i < 7; i++)
        state->regs[i] = 0;
    state->memory = initalise_memory(rom_filename);
    return 0;
}

Arcade_system initialise_system() {
    Arcade_system system;

    system.state = malloc(sizeof(Cpu_state));
    initalise_state(system.state, "rom/invaders");

    system.port = malloc(sizeof(Port));
    system.port->offset = 0;
    system.port->shift = 0;

    /*
    Cpu_state state;
    initalise_state(&state, "rom/invaders");
    system.state = &state;
    */

    //system.display = Display display;
    system.display = malloc(sizeof(Display));
    //system.display = &(Display) {};

    return system;
}

int invaders_IN(Arcade_system *system) {
    uint8_t port = system->state->memory[system->state->pc + 1];

    switch (port) {
    case 3:
        system->state->regs[A] =
            system->port->shift >> (8 - system->port->offset);
        break;
    }

    system->state->pc += 2;
    return 10;
}

int invaders_OUT(Arcade_system *system) {
    uint8_t port = system->state->memory[system->state->pc + 1];

    switch (port) {
    case 2:
        system->port->offset = system->state->regs[A] & 0x07;
        break;
    case 4:
        system->port->shift =
            (system->port->shift >> 8) | (system->state->regs[A] << 8);
        break;
    }

    system->state->pc += 2;
    return 10;
}

int invaders_op(Arcade_system *system) {
    unsigned char op_code = system->state->memory[system->state->pc];
    int cyc = 0;

    switch (op_code) {
    case 0xdb:
        cyc = invaders_IN(system);
        break;
    case 0xd3:
        cyc = invaders_OUT(system);
        break;
    default:
        cyc = emulate_op(system->state);
    }

    return cyc;
}

int main() {
#if CPUDIAG
    Cpu_state state;
    initalise_state(&state, "rom/cpudiag.bin");

    state.memory[0]=0xc3;
    state.memory[1]=0x00;
    state.memory[2]=0x01;
    state.memory[368] = 0x7;

    while (true)
        emulate_op(&state);
#else
    Arcade_system system = initialise_system();
    //memset(&cabinet, 0, sizeof(Cabinet));


    initialise_SDL(system.display);

    //atexit(cleanup);

    bool done = false;
    int cyc = 0;
    u_int32_t timer = 0;
    while (!done) {
        if ((SDL_GetTicks() - timer) > (1000 / FRAMERATE)) {
            timer = SDL_GetTicks();

            handleInput(system.input);

            while (cyc < CYCLES_PER_FRAME / 2)
                cyc += invaders_op(&system);

            cyc += interrupt(system.state, 1);

            while (cyc < CYCLES_PER_FRAME)
                cyc += invaders_op(&system);

            cyc += interrupt(system.state, 2);

            cyc = CYCLES_PER_FRAME - cyc;

            prepareScene(system.display, system.state->memory);
            presentScene(system.display);
        }
    }

    //TODO free structs
#endif

    return 0;
}
