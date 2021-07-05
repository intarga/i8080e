#include "input.h"

void keyHandler(SDL_KeyboardEvent *event, Input *input, int down) {
    (void)input;
    if (event->repeat == 0) {
        switch (event->keysym.scancode) {
        case SDL_SCANCODE_LEFT:
            input->left1 = down;
            break;
        case SDL_SCANCODE_RIGHT:
            input->right1 = down;
            break;
        case SDL_SCANCODE_UP:
            input->shot1 = down;
            break;
        case SDL_SCANCODE_A:
            input->left2 = down;
            break;
        case SDL_SCANCODE_D:
            input->right2 = down;
            break;
        case SDL_SCANCODE_W:
            input->shot2 = down;
            break;
        case SDL_SCANCODE_1:
            input->start1 = down;
            break;
        case SDL_SCANCODE_2:
            input->start2 = down;
            break;
        case SDL_SCANCODE_C:
            input->coin = down;
            break;
        default:
            break;
        }
    }
}

void handleInput(Input *input) {
    input++;
    /*
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT: exit(0); break;
        case SDL_KEYDOWN: keyHandler(&event.key, input, 1); break;
        case SDL_KEYUP: keyHandler(&event.key, input, 0); break;
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
    */
}
