#include <SDL2/SDL.h>

#define SCREEN_WIDTH  224
#define SCREEN_HEIGHT 256

typedef struct {
    SDL_Renderer *renderer;
    SDL_Window *window;
} Display;

void initialise_SDL(Display *display);
void prepareScene(Display *display, u_int8_t *memory);
void presentScene(Display *display);
