#include <SDL2/SDL.h>

typedef struct {
    int up;
    int down;
    int left;
    int right;
} Input;

void keyUpHandler(SDL_KeyboardEvent *event, Input *input);
void keyDownHandler(SDL_KeyboardEvent *event, Input *input);
void handleInput(Input *input);
