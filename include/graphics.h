#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <SDL2/SDL.h>

// colors
#define RGB_Red ((SDL_Color){255, 0, 0, 255})
#define RGB_Green ((SDL_Color){0, 255, 0, 255})
#define RGB_Blue ((SDL_Color){0, 0, 255, 255})
#define RGB_White ((SDL_Color){255, 255, 255, 255})
#define RGB_Yellow ((SDL_Color){255, 255, 0, 255})
#define RGB_Ceiling ((SDL_Color){50, 50, 50, 255})
#define RGB_Floor ((SDL_Color){100, 100, 100, 255})

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  char *title;
  int window_width;
  int window_height;
} Game;

int SDL_cleanup(Game *game, int exit_status);
int SDL_initialize(Game *game);

// this draws the walls
static inline void verLine(SDL_Renderer *renderer, int x, int y1, int y2,
                           SDL_Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderDrawLine(renderer, x, y1, x, y2);
}

#endif
