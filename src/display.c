#include "display.h"

void initialise_SDL(Display *display) {
    int rendererFlags = SDL_RENDERER_ACCELERATED;
    int windowFlags = 0;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    display->window = SDL_CreateWindow(
            "Invaders",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH, SCREEN_HEIGHT,
            windowFlags);

    if (!display->window) {
        printf("Failed to open %d x %d window: %s\n", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_GetError());
        exit(1);
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    display->renderer = SDL_CreateRenderer(display->window, -1, rendererFlags);

    if (!display->renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        exit(1);
    }
}

void prepareScene(Display *display, u_int8_t *memory) {
    SDL_Point whitepoints[57344];
    SDL_Point blackpoints[57344];
    int whitecount = 0;
    int blackcount = 0;

    for (uint16_t i = 0; i < 0x1c00; i++) {
        uint8_t byte = memory[i + 0x2400];

        for(int j = 0; j < 8; j++) {
            int x = i / 0x20;
            int y = i % 0x20;
            SDL_Point newpoint = {x, y};

            if (byte & 0x01) {
                whitepoints[whitecount++] = newpoint;
            } else {
                blackpoints[blackcount++] = newpoint;
            }
        }
    }

    SDL_SetRenderDrawColor(display->renderer, 255, 255, 255, 255);
    SDL_RenderDrawPoints(display->renderer, whitepoints, whitecount);

    SDL_SetRenderDrawColor(display->renderer, 0, 0, 0, 255);
    SDL_RenderDrawPoints(display->renderer, blackpoints, blackcount);

    SDL_RenderClear(display->renderer);
}

void presentScene(Display *display) {
    SDL_RenderPresent(display->renderer);
}
