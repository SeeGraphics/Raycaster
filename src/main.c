#include "graphics.h"
#include "map.h"
#include "player.h"
#include "raycast.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define TITLE "Raycaster"

int main() {
  // Create game objects
  Player player = {22, 22, -1, 0, 0, 0.66, 5.0, 3.0, 0.002};
  Game game = {
      NULL, NULL, TITLE, WINDOW_WIDTH, WINDOW_HEIGHT,
  };

  double time = 0, oldTime = 0;

  if (SDL_initialize(&game)) {
    return SDL_cleanup(&game, EXIT_FAILURE);
  }

  SDL_SetRelativeMouseMode(SDL_TRUE);

  while (true) {
    // get delta time
    oldTime = time;
    time = SDL_GetTicks();
    double deltaTime = (time - oldTime) / 1000.0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        return SDL_cleanup(&game, EXIT_SUCCESS);
      }

      // Handle key presses
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.scancode == SDL_SCANCODE_7) {
          static bool isFullscreen = false;
          isFullscreen = !isFullscreen;
          SDL_SetWindowFullscreen(
              game.window, isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        }
        if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
          return SDL_cleanup(&game, EXIT_SUCCESS);
        }
      }

      if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          game.window_width = event.window.data1;
          game.window_height = event.window.data2;
        }
      }

      if (event.type == SDL_MOUSEMOTION) {
        player_rotate(&player, mouse_rotationAmount(player.sensitivity,
                                                    -event.motion.xrel));
      }
    }

    const Uint8 *state = SDL_GetKeyboardState(NULL);

    if (state[SDL_SCANCODE_UP] || state[SDL_SCANCODE_W])
      player_move(&player, deltaTime, worldMap, 1);
    if (state[SDL_SCANCODE_DOWN] || state[SDL_SCANCODE_S])
      player_move(&player, deltaTime, worldMap, -1);
    if (state[SDL_SCANCODE_LEFT])
      player_rotate(&player, key_rotationAmount(player.rotSpeed, deltaTime, 1));
    if (state[SDL_SCANCODE_RIGHT])
      player_rotate(&player,
                    key_rotationAmount(player.rotSpeed, deltaTime, -1));

    // strafing l+r
    if (state[SDL_SCANCODE_A])
      player_strafe(&player, deltaTime, worldMap, -1);
    if (state[SDL_SCANCODE_D])
      player_strafe(&player, deltaTime, worldMap, 1);

    // Set ceiling color and clear screen
    SDL_SetRenderDrawColor(game.renderer, RGB_Ceiling.r, RGB_Ceiling.g,
                           RGB_Ceiling.b, RGB_Ceiling.a);
    SDL_RenderClear(game.renderer);

    // Draw floor
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
