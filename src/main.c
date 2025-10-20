#include "font.h"
#include "graphics.h"
#include "map.h"
#include "player.h"
#include "raycast.h"
#include "texture.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define TITLE "Raycaster"

int main() {
  // Create game objects
  Game game = {NULL, NULL, NULL, TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, NULL};
  Player player = {22, 22, -1, 0, 0, 0.88, 5.0, 3.0, 0.002};
  TextureManager textureManager = {NULL};

  // delta time variables
  double time = 0, oldTime = 0;

  // Initialization
  if (SDL_initialize(&game)) {
    return SDL_cleanup(&game, EXIT_FAILURE);
  }

  // allocate screen buffer
  game.buffer =
      malloc(game.window_width * game.window_height * sizeof(uint32_t));
  if (!game.buffer) {
    fprintf(stderr, "Couldn't allocate buffer");
    SDL_cleanup(&game, EXIT_FAILURE);
    return 1;
  }

  // allocate textures
  for (int i = 0; i < NUM_TEXTURES; i++) {
    textureManager.textures[i] =
        malloc(TEXT_WIDTH * TEXT_HEIGHT * sizeof(uint32_t));
    if (!textureManager.textures[i]) {
      fprintf(stderr, "Couldn't allocate texture %d", i);
      free(textureManager.textures[i]);
      return 1;
    }
  }
  loadTextures(&textureManager, TEXT_WIDTH, TEXT_HEIGHT);

  // Font loading
  if (TTF_Init() == -1) {
    printf("TTF_Init failed: %s\n", TTF_GetError());
    return EXIT_FAILURE;
  }
  Font font = {
      TTF_OpenFont("assets/font/Doom.ttf", 120),
      TTF_OpenFont("assets/font/Doom.ttf", 60),
      TTF_OpenFont("assets/font/Doom.ttf", 90),
  };
  if (!font.debug || !font.ui || !font.title) {
    fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
    return EXIT_FAILURE;
  }

  // FPS Mouse
  SDL_SetRelativeMouseMode(SDL_TRUE);

  while (true) {

    // get delta time
    oldTime = time;
    time = SDL_GetTicks();
    double deltaTime = (time - oldTime) / 1000.0;

    // Calculate FPS
    int fps = (deltaTime > 0) ? (int)(1.0 / deltaTime) : 0;

    // Quit
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

      // resizing window
      if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          game.window_width = event.window.data1;
          game.window_height = event.window.data2;

          // free & reallocate buffer for textures
          free(game.buffer);
          game.buffer =
              malloc(game.window_width * game.window_height * sizeof(uint32_t));
          if (!game.buffer) {
            fprintf(stderr, "Couldn't allocate buffer");
            SDL_cleanup(&game, EXIT_FAILURE);
            return 1;
          }

          // recreate texture
          if (game.screen_texture) {
            SDL_DestroyTexture(game.screen_texture);
          }
          game.screen_texture =
              SDL_CreateTexture(game.renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING, game.window_width,
                                game.window_height);
          if (!game.screen_texture) {
            fprintf(stderr, "Failed to recreate texture: %s\n", SDL_GetError());
            SDL_cleanup(&game, EXIT_FAILURE);
            return 1;
          }
        }
      }

      // mouse control
      if (event.type == SDL_MOUSEMOTION) {
        player_rotate(&player, mouse_rotationAmount(player.sensitivity,
                                                    -event.motion.xrel));
      }
    }

    // Input
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_Q] && SDL_GetRelativeMouseMode())
      SDL_SetRelativeMouseMode(SDL_FALSE);
    else if (state[SDL_SCANCODE_Q] && !SDL_GetRelativeMouseMode())
      SDL_SetRelativeMouseMode(SDL_TRUE);
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

    // Clear buffer
    for (int y = 0; y < game.window_height; y++) {
      for (int x = 0; x < game.window_width; x++) {
        game.buffer[y * game.window_width + x] = 0;
      }
    }

    // draw game
    perform_floorcasting(&game, &textureManager, &player); // floor and roof
    perform_raycasting(&game, &textureManager, &player);
    drawBuffer(&game);

    // Draw FPS counter
    char fpsText[32];
    snprintf(fpsText, sizeof(fpsText), "FPS: %d", fps);
    renderText(game.renderer, font.debug, fpsText, 10, 10, RGB_Yellow);

    SDL_RenderPresent(game.renderer);
    SDL_Delay(2); // ~140 fps
  }

  // cleanup
  free(textureManager.textures);
  free(game.buffer);
  TTF_CloseFont(font.title);
  TTF_CloseFont(font.ui);
  TTF_CloseFont(font.debug);

  return SDL_cleanup(&game, EXIT_SUCCESS);
}
