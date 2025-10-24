#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define TITLE "Raycaster"

// render to small internal Res
#define RENDER_WIDTH 800
#define RENDER_HEIGHT 400

// colors
#define RGB_Red ((SDL_Color){255, 0, 0, 255})
#define RGB_Green ((SDL_Color){0, 255, 0, 255})
#define RGB_Blue ((SDL_Color){0, 0, 255, 255})
#define RGB_White ((SDL_Color){255, 255, 255, 255})
#define RGB_Black ((SDL_Color){0, 0, 0, 255})
#define RGB_Yellow ((SDL_Color){255, 255, 0, 255})
#define RGB_Ceiling ((SDL_Color){50, 50, 50, 255})
#define RGB_Floor ((SDL_Color){100, 100, 100, 255})

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *screen_texture;
  char *title;
  int window_width;
  int window_height;
  uint32_t *buffer;
  uint32_t *Rbuffer;
  double *Zbuffer;
} Game;

Game createGame();

void clearBuffer(Game *game);
int buffers_reallocate(Game *game);
int buffers_init(Game *game);
int SDL_cleanup(Game *game, int exit_status);
int SDL_initialize(Game *game);
void drawBuffer(Game *game);

#endif
