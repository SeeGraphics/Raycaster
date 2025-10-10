#include "graphics.h"
#include "map.h"
#include "player.h"
#include "raycast.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define TITLE "Raycaster"

int main() {
  // Create game objects
  Player player = {22, 22, -1, 0, 0, 0.66, 5.0, 3.0};
  Game game = {
      NULL, NULL, TITLE, WINDOW_WIDTH, WINDOW_HEIGHT,
  };

  double time = 0, oldTime = 0;

  if (SDL_initialize(&game)) {
    return SDL_cleanup(&game, EXIT_FAILURE);
  }

  while (true) {
    // delta time
    oldTime = time;
    time = SDL_GetTicks();
    double frameTime = (time - oldTime) / 1000.0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        return SDL_cleanup(&game, EXIT_SUCCESS);
      }

      // Handle key presses (not held keys)
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.scancode == SDL_SCANCODE_7) {
          static bool isFullscreen = false;
          isFullscreen = !isFullscreen;
          SDL_SetWindowFullscreen(
              game.window, isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        }
      }

      if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          game.window_width = event.window.data1;
          game.window_height = event.window.data2;
        }
      }
    }

    const Uint8 *state = SDL_GetKeyboardState(NULL);

    // move forward/backward
    if (state[SDL_SCANCODE_UP]) {
      player_move_forward(&player, frameTime, worldMap);
    }
    if (state[SDL_SCANCODE_DOWN]) {
      player_move_backward(&player, frameTime, worldMap);
    }

    // rotate left/right
    if (state[SDL_SCANCODE_LEFT]) {
      player_rotate_left(&player, frameTime);
    }
    if (state[SDL_SCANCODE_RIGHT]) {
      player_rotate_right(&player, frameTime);
    }
    // Set ceiling color and clear screen
    SDL_SetRenderDrawColor(game.renderer, RGB_Ceiling.r, RGB_Ceiling.g,
                           RGB_Ceiling.b, RGB_Ceiling.a);
    SDL_RenderClear(game.renderer);

    // Draw floor (bottom half of screen)
    SDL_SetRenderDrawColor(game.renderer, RGB_Floor.r, RGB_Floor.g, RGB_Floor.b,
                           RGB_Floor.a);

    SDL_Rect floor = {0, game.window_height / 2, game.window_width,
                      game.window_height / 2};
    SDL_RenderFillRect(game.renderer, &floor);

    // draw game
    perform_raycasting(&game, &player);
    SDL_RenderPresent(game.renderer);
    SDL_Delay(16);
  }

  return SDL_cleanup(&game, EXIT_SUCCESS);
}
