#include "graphics.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

int SDL_initialize(Game *game) {
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    fprintf(stderr, "SDL failed to initialize: %s\n", SDL_GetError());
    return 1;
  }
  printf("SDL loaded...\n");

  game->window = SDL_CreateWindow(
      game->title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      game->window_width, game->window_height,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALWAYS_ON_TOP);
  if (!game->window) {
    fprintf(stderr, "SDL failed to create window: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  game->renderer = SDL_CreateRenderer(game->window, -1, 0);
  if (!game->renderer) {
    fprintf(stderr, "SDL failed to create renderer: %s\n", SDL_GetError());
    SDL_DestroyWindow(game->window);
    SDL_Quit();
    return 1;
  }

  return 0;
}

int SDL_cleanup(Game *game, int exit_status) {
  if (game->renderer)
    SDL_DestroyRenderer(game->renderer);
  if (game->window)
    SDL_DestroyWindow(game->window);
  SDL_Quit();
  return exit_status;
}
