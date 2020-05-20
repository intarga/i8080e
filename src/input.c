#include "input.h"

void keyUpHandler(SDL_KeyboardEvent *event, Input *input) {
    (void)input;
    if (event->repeat == 0) {
        if (event->keysym.scancode == SDL_SCANCODE_UP)
            input->up = 0;

        if (event->keysym.scancode == SDL_SCANCODE_DOWN)
            input->down = 0;

        if (event->keysym.scancode == SDL_SCANCODE_LEFT)
            input->left = 0;

        if (event->keysym.scancode == SDL_SCANCODE_RIGHT)
            input->right = 0;
    }
}

void keyDownHandler(SDL_KeyboardEvent *event, Input *input) {
    (void)input;
    if (event->repeat == 0) {
        if (event->keysym.scancode == SDL_SCANCODE_UP)
            input->up = 1;

        if (event->keysym.scancode == SDL_SCANCODE_DOWN)
            input->down = 1;

        if (event->keysym.scancode == SDL_SCANCODE_LEFT)
            input->left = 1;

        if (event->keysym.scancode == SDL_SCANCODE_RIGHT)
            input->right = 1;
    }
}

void handleInput(Input *input) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT: exit(0); break;
        case SDL_KEYDOWN: keyDownHandler(&event.key, input); break;
        case SDL_KEYUP: keyUpHandler(&event.key, input); break;
        }
    }

    if (input->up) {
        //TODO
    }

    if (input->down) {
        //TODO
    }

    if (input->left) {
        //TODO
    }

    if (input->right) {
        //TODO
    }
}
