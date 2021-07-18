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

void load_rom_file(char *filename, uint8_t *memory) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("Could not open %s\n", filename);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    int bytes_read;
    bytes_read = fread(memory, 1, fsize, f);
    fclose(f);

    if (bytes_read != fsize) {
        printf("Failed to fully read file: %s\n", filename);
        exit(1);
    }
}

uint8_t *initalise_memory(char *rom_path) {
    uint8_t *memory = malloc(sizeof(uint8_t) * 0x4000);
    memset(memory, 0, 0x4000);

#if CPUDIAG
    load_rom_file(rom_path, &memory[0x100]);
#else
    char filepath[100];

    strcpy(filepath, rom_path);
    strcat(filepath, "/invaders.h");
    load_rom_file(filepath, &memory[0x0000]);

    strcpy(filepath, rom_path);
    strcat(filepath, "/invaders.g");
    load_rom_file(filepath, &memory[0x0800]);

    strcpy(filepath, rom_path);
    strcat(filepath, "/invaders.f");
    load_rom_file(filepath, &memory[0x1000]);

    strcpy(filepath, rom_path);
    strcat(filepath, "/invaders.e");
    load_rom_file(filepath, &memory[0x1800]);
#endif

    return memory;
}

int initalise_state(Cpu_state *state, char *rom_path) {
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
    state->memory = initalise_memory(rom_path);
    return 0;
}

Arcade_system initialise_system() {
    Arcade_system system;

    system.state = malloc(sizeof(Cpu_state));
    initalise_state(system.state, "rom");

    system.input = malloc(sizeof(Input));
    system.input->left1 = 0;
    system.input->right1 = 0;
    system.input->shot1 = 0;
    system.input->start1 = 0;
    system.input->left2 = 0;
    system.input->right2 = 0;
    system.input->shot2 = 0;
    system.input->start2 = 0;
    system.input->coin = 0;
    system.input->quit = 0;

    system.port = malloc(sizeof(Port));
    system.port->offset = 0;
    system.port->shift = 0;

    system.display = malloc(sizeof(Display));

    return system;
}

int invaders_IN(Arcade_system *system) {
    uint8_t port = read_memory(system->state, system->state->pc + 1);

    switch (port) {
    case 1:
        system->state->regs[A] = system->input->coin
            | (system->input->start2 << 1)
            | (system->input->start1 << 2)
            | (1 << 3)
            | (system->input->shot1 << 4)
            | (system->input->left1 << 5)
            | (system->input->right1 << 6);
        break;
    case 2:
        system->state->regs[A] = (system->input->shot2 << 4)
            | (system->input->left2 << 5)
            | (system->input->right2 << 6);
        break;
    case 3:
        system->state->regs[A] =
            system->port->shift >> (8 - system->port->offset);
        break;
    }

    system->state->pc += 2;
    return 10;
}

int invaders_OUT(Arcade_system *system) {
    uint8_t port = read_memory(system->state, system->state->pc + 1);

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
    uint8_t op_code = read_memory(system->state, system->state->pc);
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

void cleanup(Arcade_system system) {
    SDL_DestroyTexture(system.display->texture);
    SDL_DestroyRenderer(system.display->renderer);
    SDL_DestroyWindow(system.display->window);
    SDL_Quit();

    free(system.state->memory);
    free(system.state);
    free(system.input);
    free(system.port);
    free(system.display);
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

    initialise_SDL(system.display);

    //atexit(cleanup);

    int cyc = 0;
    u_int32_t timer = 0;
    while (!system.input->quit) {
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

    cleanup(system);
#endif

    return 0;
}
