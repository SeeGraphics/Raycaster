#include "input.h"
#include "animation.h"

int mouseUngrabbed = 0;

int handleInput(Engine *engine, double deltaTime) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {

    /* WINDOW EVENTS */
    if (event.type == SDL_QUIT) {
      engine_cleanup(engine, EXIT_SUCCESS);
      return 1;
    }

    // Window resize
    if (event.type == SDL_WINDOWEVENT &&
        event.window.event == SDL_WINDOWEVENT_RESIZED) {

      engine->game.window_width = event.window.data1;
      engine->game.window_height = event.window.data2;

      // Reallocate buffers
      buffers_reallocate(&engine->game);

      // Recreate texture with new dimensions
      if (engine->game.screen_texture) {
        SDL_DestroyTexture(engine->game.screen_texture);
      }

      engine->game.screen_texture = SDL_CreateTexture(
          engine->game.renderer, SDL_PIXELFORMAT_ARGB8888,
          SDL_TEXTUREACCESS_STREAMING, RENDER_WIDTH, RENDER_HEIGHT);

      if (!engine->game.screen_texture) {
        fprintf(stderr, "[ERROR] Failed to recreate texture: %s\n",
                SDL_GetError());
        engine_cleanup(engine, EXIT_FAILURE);
        return 1;
      }

      printf("[WINDOW] Window resized to %dx%d\n", engine->game.window_width,
             engine->game.window_height);
    }

    /* KEY EVENTS */
    if (event.type == SDL_KEYDOWN) {
      // Fullscreen toggle
      if (event.key.keysym.scancode == TOGGLE_FULLSCREEN) {
        static bool isFullscreen = false;
        isFullscreen = !isFullscreen;
        SDL_SetWindowFullscreen(engine->game.window,
                                isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP
                                             : 0);
      }

      // Quit
      if (event.key.keysym.scancode == CLOSE_GAME_ESC) {
        engine_cleanup(engine, EXIT_SUCCESS);
        return 1;
      }

      // ungrab Mouse
      if (event.key.keysym.scancode == UNGRAB_MOUSE && mouseUngrabbed == 0) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        mouseUngrabbed = 1;
      } else if (event.key.keysym.scancode == UNGRAB_MOUSE &&
                 mouseUngrabbed == 1) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
        mouseUngrabbed = 0;
      }

      // Reload
      if (event.key.keysym.scancode == GUN_RELOAD) {
        playShotgunReload(&engine->sound);
      }
    }

    /* MOUSE EVENTS */
    if (event.type == SDL_MOUSEWHEEL && !engine->player.shooting) {
      if (event.wheel.y > 0) {
        // Scroll up -> next gun
        engine->player.selectedGun =
            (engine->player.selectedGun + 1) % engine->player.gunsTotal;
      } else if (event.wheel.y < 0) {
        // Optional: scroll down ->  previous gun
        engine->player.selectedGun =
            (engine->player.selectedGun - 1 + engine->player.gunsTotal) %
            engine->player.gunsTotal;
      }
    }

    if (event.type == SDL_MOUSEMOTION) {
      // Horizontal rotation
      player_rotate(&engine->player, mouse_rotationAmount(engine->player.sensX,
                                                          -event.motion.xrel));

      // Vertical pitch
      engine->player.pitch -=
          event.motion.yrel * engine->player.sensX * engine->player.sensY;

      // Clamp pitch
      if (engine->player.pitch > CLAMP)
        engine->player.pitch = CLAMP;
      else if (engine->player.pitch < -CLAMP)
        engine->player.pitch = -CLAMP;
    }

    if (event.type == SDL_MOUSEBUTTONDOWN) {
      if (event.button.button == MSB_LEFT) {
        switch (engine->player.selectedGun) {
        case SHOTGUN:
          if (!animations.shotgun_shoot.playing) {
            animations.shotgun_shoot.playing = 1;
            playShotgunShot(&engine->sound);
          }
          break;
        case ROCKET:
          if (!animations.rocket_shoot.playing) {
            animations.rocket_shoot.playing = 1;
            playRocketShot(&engine->sound);
          }
          break;
        case PISTOL:
          if (!animations.pistol_shoot.playing) {
            animations.pistol_shoot.playing = 1;
            playPistolShot(&engine->sound);
          }
          break;
        case HANDS:
          if (!animations.hands_punsh.playing) {
            animations.hands_punsh.playing = 1;
            playHandsPunsh(&engine->sound);
          }
          break;
        case SINGLE:
          if (!animations.single_shoot.playing) {
            animations.single_shoot.playing = 1;
            playShotgunShot(&engine->sound);
          }
          break;
        default:
          break;
        }
      }
    }
  }

  /* CONTINUOUS INPUT (held keys) */
  const Uint8 *state = SDL_GetKeyboardState(NULL);

  // Movement
  if (state[SDL_SCANCODE_W] || state[SDL_SCANCODE_UP])
    player_move(&engine->player, deltaTime, worldMap, 1);
  if (state[SDL_SCANCODE_S] || state[SDL_SCANCODE_DOWN])
    player_move(&engine->player, deltaTime, worldMap, -1);
  if (state[SDL_SCANCODE_A])
    player_strafe(&engine->player, deltaTime, worldMap, -1);
  if (state[SDL_SCANCODE_D])
    player_strafe(&engine->player, deltaTime, worldMap, 1);

  // Rotation with keys
  if (state[SDL_SCANCODE_LEFT])
    player_rotate(&engine->player,
                  key_rotationAmount(engine->player.rotSpeed, deltaTime, 1));
  if (state[SDL_SCANCODE_RIGHT])
    player_rotate(&engine->player,
                  key_rotationAmount(engine->player.rotSpeed, deltaTime, -1));

  return 0;
}
