#include "font.h"
#include "graphics.h"
#include "gun.h"
#include "map.h"
#include "player.h"
#include "raycast.h"
#include "sound.h"
#include "sprites.h"
#include "texture.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define TITLE "Raycaster"

int main() {
  // Create game objects
  Game game = {NULL,         NULL,          NULL, TITLE,
               WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL};
  Player player = {POS_X,      POS_Y,     DIR_X,  DIR_Y,  PLANE_X, PLANE_Y,
                   MOVE_SPEED, ROT_SPEED, SENS_X, SENS_Y, PITCH};
  TextureManager textureManager = {NULL};
  SoundManager soundManager = {NULL, NULL};
  Sprite sprite[NUM_SPRITES] = {
      // TODO: put in function createSprites()
      // green lights
      {20.5, 11.5, 10},
      {18.5, 4.5, 10},
      {10.0, 4.5, 10},
      {10.0, 12.5, 10},
      {3.5, 6.5, 10},
      {3.5, 20.5, 10},
      {3.5, 14.5, 10},
      {14.5, 20.5, 10},

      // row of pillars
      {18.5, 10.5, 11},
      {18.5, 11.5, 8},
      {18.5, 12.5, 8},

      // barrels
      {21.5, 1.5, 9},
      {15.5, 1.5, 9},
      {16.0, 1.8, 9},
      {16.2, 1.2, 9},
      {3.5, 2.5, 9},
      {9.5, 15.5, 9},
      {10.0, 15.1, 9},
      {10.5, 15.8, 9},
  };

  // delta time variables
  double time = 0, oldTime = 0;

  // Initialization
  if (SDL_initialize(&game)) {
    return SDL_cleanup(&game, EXIT_FAILURE);
  }

  // Initialize sound system
  if (initSound() < 0) {
    fprintf(stderr, "Failed to initialize sound\n");
    return SDL_cleanup(&game, EXIT_FAILURE);
  }
  loadSounds(&soundManager);

  // allocate screen buffer -- TODO: helper function here too
  game.buffer =
      malloc(game.window_width * game.window_height * sizeof(uint32_t));
  if (!game.buffer) {
    fprintf(stderr, "Couldn't allocate buffer");
    SDL_cleanup(&game, EXIT_FAILURE);
    return 1;
  }

  // allocate Zbuffer -- TODO: create helper function for this
  game.Zbuffer = malloc(game.window_width * sizeof(double));
  if (!game.Zbuffer) {
    fprintf(stderr, "Couldn't allocate Zbuffer");
    SDL_cleanup(&game, EXIT_FAILURE);
    return 1;
  }

  // allocate textures -- TODO: helper function
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

  // load all gun frames
  loadAllTextures_Shotgun_shoot(game.renderer);
  loadAllTextures_Shotgun_reload(game.renderer);
  GunAnim gunAnim = {0, 0.10, 0.0, 0, gunTextures, SHOTGUN_SHOOT_FRAMES};

  // Font loading -- TODO: helper function
  if (TTF_Init() == -1) {
    printf("TTF_Init failed: %s\n", TTF_GetError());
    return EXIT_FAILURE;
  }
  Font font = {
      TTF_OpenFont("assets/font/Doom.ttf", 120), // title
      TTF_OpenFont("assets/font/Doom.ttf", 40),  // debug
      TTF_OpenFont("assets/font/Doom.ttf", 90),  // UI
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
        cleanupGunTextures();
        cleanupSound(&soundManager);
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
          cleanupGunTextures();
          cleanupSound(&soundManager);
          return SDL_cleanup(&game, EXIT_SUCCESS);
        }
        if (event.key.keysym.scancode == SDL_SCANCODE_R &&
            gunAnim.playing == 0) {
          gunAnim.playing = 1;
          gunAnim.currentFrame = 0;
          gunAnim.timeAccumulator = 0.0;
          gunAnim.currentAnim = reloadTextures;
          gunAnim.maxFrames = SHOTGUN_RELOAD_FRAMES;
          playShotgunReload(&soundManager);
        }
      }

      // resizing window
      if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          game.window_width = event.window.data1;
          game.window_height = event.window.data2;

          // free & reallocate buffer for textures -- TODO: helper function
          free(game.buffer);
          game.buffer =
              malloc(game.window_width * game.window_height * sizeof(uint32_t));
          if (!game.buffer) {
            fprintf(stderr, "Couldn't allocate buffer");
            SDL_cleanup(&game, EXIT_FAILURE);
            return 1;
          }
          free(game.Zbuffer);
          game.Zbuffer = malloc(game.window_width * sizeof(double));
          if (!game.Zbuffer) {
            fprintf(stderr, "Couldn't allocate Zbuffer");
            SDL_cleanup(&game, EXIT_FAILURE);
            return 1;
          }

          // recreate texture -- TODO: helper function
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
        player_rotate(&player,
                      mouse_rotationAmount(player.sensX, -event.motion.xrel));
        // TODO: helper function
        player.pitch -= event.motion.yrel * player.sensX * player.sensY;
        if (player.pitch > CLAMP)
          player.pitch = CLAMP;
        else if (player.pitch < -CLAMP)
          player.pitch = -CLAMP;
      }
      if (event.type == SDL_MOUSEBUTTONDOWN &&
          event.button.button == SDL_BUTTON_LEFT && gunAnim.playing == 0) {
        gunAnim.playing = 1;
        gunAnim.currentFrame = 0;
        gunAnim.timeAccumulator = 0.0;
        gunAnim.currentAnim = gunTextures;
        gunAnim.maxFrames = SHOTGUN_SHOOT_FRAMES;
        playShotgunShot(&soundManager);
      }
    }

    // update animation
    if (gunAnim.playing) {
      gunAnim.timeAccumulator += deltaTime;
      if (gunAnim.timeAccumulator >= gunAnim.frameTime) {
        gunAnim.timeAccumulator -= gunAnim.frameTime;
        gunAnim.currentFrame++;

        // Stop animation when finished
        if (gunAnim.currentFrame >= gunAnim.maxFrames) {
          gunAnim.currentFrame = 0;
          gunAnim.playing = 0;

          gunAnim.currentAnim = gunTextures;
          gunAnim.maxFrames = SHOTGUN_SHOOT_FRAMES;
        }
      }
    }

    // Input TODO: Mabye put this in player?
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
    perform_floorcasting(&game, &textureManager, &player);
    perform_raycasting(&game, &textureManager, &player);
    perform_spritecasting(&game, sprite, &textureManager, &player);
    drawBuffer(&game);

    // Draw the current gun frame
    if (gunAnim.currentAnim && gunAnim.currentAnim[gunAnim.currentFrame]) {
      drawGunTexture(&game, gunAnim.currentAnim[gunAnim.currentFrame], 0.4f,
                     0.6f);
    }

    // draw FPS counter
    renderInt(game.renderer, font.debug, "FPS", fps, 10, 10, RGB_Yellow);
    // draw Coordinates
    renderFloatPair(game.renderer, font.debug, "POS", player.posX, player.posY,
                    10, 50, RGB_Yellow);
    // draw direction
    renderFloatPair(game.renderer, font.debug, "DIR", player.dirX, player.dirY,
                    10, 90, RGB_Yellow);
    // draw pitch
    renderFloat(game.renderer, font.debug, "PITCH", player.pitch, 10, 170,
                RGB_Yellow);
    // draw plane
    renderFloatPair(game.renderer, font.debug, "PLANE", player.planeX,
                    player.planeY, 10, 130, RGB_Yellow);

    SDL_RenderPresent(game.renderer);
  }

  // cleanup
  for (int i = 0; i < NUM_TEXTURES; i++) {
    free(textureManager.textures[i]);
  }
  free(game.buffer);
  free(game.Zbuffer);
  TTF_CloseFont(font.title);
  TTF_CloseFont(font.ui);
  TTF_CloseFont(font.debug);
  cleanupGunTextures();
  cleanupSound(&soundManager);

  return SDL_cleanup(&game, EXIT_SUCCESS);
}
