#include "graphics.h"
#include "types.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

Game createGame() {
  Game g = {NULL,          NULL, NULL, TITLE, WINDOW_WIDTH,
            WINDOW_HEIGHT, NULL, NULL, NULL};
  return g;
}

int buffers_reallocate(Game *game) {
  free(game->buffer);
  game->buffer = malloc(game->window_width * game->window_height * sizeof(u32));
  if (!game->buffer) {
    fprintf(stderr, "[ERROR] Couldn't allocate buffer");
    SDL_cleanup(game, EXIT_FAILURE);
    return 1;
  }
  free(game->Rbuffer);
  game->Rbuffer = malloc(RENDER_WIDTH * RENDER_HEIGHT * sizeof(u32));
  if (!game->Rbuffer) {
    fprintf(stderr, "[ERROR] Couldn't allocate Rbuffer");
    SDL_cleanup(game, EXIT_FAILURE);
    return 1;
  }
  free(game->Zbuffer);
  game->Zbuffer = malloc(RENDER_WIDTH * sizeof(double));
  if (!game->Zbuffer) {
    fprintf(stderr, "[ERROR] Couldn't allocate Zbuffer");
    SDL_cleanup(game, EXIT_FAILURE);
    return 1;
  }
  return 0;
}

void clearBuffer(Game *game) {
  if (!game || !game->buffer || !game->Rbuffer) {
    fprintf(stderr, "[ERROR] clearBuffer called with NULL buffer!\n");
    return;
  }

  for (int y = 0; y < RENDER_HEIGHT; y++) {
    for (int x = 0; x < RENDER_WIDTH; x++) {
      game->Rbuffer[y * RENDER_WIDTH + x] = 0;
    }
  }

  for (int y = 0; y < game->window_height; y++) {
    for (int x = 0; x < game->window_width; x++) {
      game->buffer[y * game->window_width + x] = 0;
    }
  }
}

int buffers_init(Game *game) {
  // game pixel buffer (main buffer we draw to)
  game->buffer = malloc(game->window_width * game->window_height * sizeof(u32));
  if (!game->buffer) {
    fprintf(stderr, "[ERROR] Couldn't allocate buffer");
    SDL_cleanup(game, EXIT_FAILURE);
    return 1;
  }

  // renderBuffer for performance
  game->Rbuffer = malloc(RENDER_WIDTH * RENDER_HEIGHT * sizeof(u32));
  if (!game->Rbuffer) {
    fprintf(stderr, "[ERROR] Couldn't allocate Rbuffer");
    SDL_cleanup(game, EXIT_FAILURE);
    return 1;
  }

  // Z-index for sprites...
  game->Zbuffer = malloc(RENDER_WIDTH * sizeof(double));
  if (!game->Zbuffer) {
    fprintf(stderr, "[ERROR] Couldn't allocate Zbuffer");
    SDL_cleanup(game, EXIT_FAILURE);
    return 1;
  }
  return 0;
}

int SDL_initialize(Game *game) {
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    fprintf(stderr, "[ERROR] SDL failed to initialize: %s\n", SDL_GetError());
    return 1;
  }
  printf("[SDL] SDL loaded...\n");

  game->window = SDL_CreateWindow(
      game->title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      game->window_width, game->window_height,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALWAYS_ON_TOP);
  if (!game->window) {
    fprintf(stderr, "[ERROR] SDL failed to create window: %s\n",
            SDL_GetError());
    SDL_Quit();
    return 1;
  }

  game->renderer = SDL_CreateRenderer(game->window, -1, 0);
  if (!game->renderer) {
    fprintf(stderr, "[ERROR] SDL failed to create renderer: %s\n",
            SDL_GetError());
    SDL_DestroyWindow(game->window);
    SDL_Quit();
    return 1;
  }

  // Create texture for buffer at window size (NOT RENDER SIZE)
  game->screen_texture = SDL_CreateTexture(
      game->renderer,
      SDL_PIXELFORMAT_ARGB8888, // 32-bit color format
      SDL_TEXTUREACCESS_STREAMING, RENDER_WIDTH, RENDER_HEIGHT);

  if (!game->screen_texture) {
    fprintf(stderr, "[ERROR] Failed to create texture: %s\n", SDL_GetError());
    return 1;
  }

  return 0;
}

int SDL_cleanup(Game *game, int exit_status) {
  // Destroy textures first (they depend on the renderer)
  if (game->screen_texture) {
    SDL_DestroyTexture(game->screen_texture);
    game->screen_texture = NULL;
  }

  // Then destroy the renderer
  if (game->renderer) {
    SDL_DestroyRenderer(game->renderer);
    game->renderer = NULL;
  }

  // Then destroy the window
  if (game->window) {
    SDL_DestroyWindow(game->window);
    game->window = NULL;
  }

  SDL_Quit();
  return exit_status;
}

void drawBuffer(Game *game) {
  // Update texture with buffer data
  SDL_UpdateTexture(game->screen_texture,
                    NULL, // Update entire texture
                    game->Rbuffer,
                    RENDER_WIDTH * sizeof(u32) // Pitch (bytes per row)
  );

  /* Here we use Rbuffer (low res) to create the texture, then create a
   * rectangle and the top left of the screen (0, 0) and with the size of
   * whatever the windows size currently is then copy the texture to the screen.
   * By passing in a Rect, SDL will automatically scale the texture to the Rect
   * which has the size of the window */
  SDL_Rect dstRect = {0, 0, game->window_width, game->window_height};
  SDL_RenderCopy(game->renderer, game->screen_texture, NULL, &dstRect);
}
