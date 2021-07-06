#include "display.h"
#include <SDL2/SDL_render.h>

void initialise_SDL(Display *display) {
    int rendererFlags = SDL_RENDERER_ACCELERATED;
    int windowFlags = SDL_WINDOW_RESIZABLE;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    display->window = SDL_CreateWindow(
            "Invaders",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            SCREEN_WIDTH * 2,
            SCREEN_HEIGHT * 2,
            windowFlags);

    if (!display->window) {
        printf(
                "Failed to open %d x %d window: %s\n",
                SCREEN_WIDTH * 2,
                SCREEN_HEIGHT * 2,
                SDL_GetError());
        exit(1);
    }

    SDL_SetWindowMinimumSize(display->window, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_ShowCursor(SDL_DISABLE);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    display->renderer = SDL_CreateRenderer(display->window, -1, rendererFlags);

    if (!display->renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_RenderSetLogicalSize(display->renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    display->texture = SDL_CreateTexture(
            display->renderer,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STREAMING,
            SCREEN_WIDTH,
            SCREEN_HEIGHT);

    if (!display->texture) {
        printf("Failed to create texture: %s\n", SDL_GetError());
        exit(1);
    }
}

void prepareScene(Display *display, u_int8_t *memory) {
    for (uint16_t i = 0; i < 0x1c00; i++) {
        uint8_t byte = memory[i + 0x2400];

        for(int bit = 0; bit < 8; bit++) {
            int x = 255 - ((i * 8) % 256 + bit); //implicit rotation between
            int y = i * 8 / 256; //buffer and texture

            uint8_t r = 0;
            uint8_t g = 0;
            uint8_t b = 0;

            if ((byte >> bit) & 0x01) { //reverse bitshift??
                r = 255;
                g = 255;
                b = 255;
            }

            display->pixels[x][y][0] = r;
            display->pixels[x][y][1] = g;
            display->pixels[x][y][2] = b;
            display->pixels[x][y][3] = 255;//offload?
        }
    }

    SDL_UpdateTexture(
            display->texture,
            NULL,
            &display->pixels,
            sizeof(uint8_t) * 4 * SCREEN_WIDTH);

    SDL_RenderClear(display->renderer);
    SDL_RenderCopy(display->renderer, display->texture, NULL, NULL);
}

void presentScene(Display *display) {
    SDL_RenderPresent(display->renderer);
}
