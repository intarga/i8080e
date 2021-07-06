#include <SDL2/SDL.h>

typedef struct {
    int left1;
    int right1;
    int shot1;
    int start1;
    int left2;
    int right2;
    int shot2;
    int start2;
    int coin;
    int quit;
} Input;

void keyUpHandler(SDL_KeyboardEvent *event, Input *input);
void keyDownHandler(SDL_KeyboardEvent *event, Input *input);
void handleInput(Input *input);
