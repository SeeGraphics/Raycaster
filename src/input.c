#include "input.h"
#include "animation.h"
#include "player.h"
#include "enemies.h"
#include "weapons.h"
#include "entities.h"
#include <math.h>

int mouseUngrabbed = 0;

int handleInput(Engine *engine, double deltaTime) {
  SDL_Event event;
  engine->player.velX = 0.0;
  engine->player.velY = 0.0;
  bool allowGameplayInput = !engine_isTransitionActive(engine);

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
        fprintf(stderr,
                "\033[31m[ERROR] Failed to recreate texture: %s\033[0m\n",
                SDL_GetError());
        engine_cleanup(engine, EXIT_FAILURE);
        return 1;
      }

      printf("\033[32m[WINDOW] Window resized to %dx%d\033[0m\n",
             engine->game.window_width, engine->game.window_height);
    }

    /* KEY EVENTS */
    if (event.type == SDL_KEYDOWN) {
      SDL_Scancode sc = event.key.keysym.scancode;

      if (sc == SDL_SCANCODE_R && engine->player.health <= 0) {
        engine_reloadLevel(engine);
        continue;
      }

      // Fullscreen toggle
      if (sc == TOGGLE_FULLSCREEN) {
        static bool isFullscreen = false;
        isFullscreen = !isFullscreen;
        SDL_SetWindowFullscreen(engine->game.window,
                                isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP
                                             : 0);
      }

      // Quit
      if (sc == CLOSE_GAME_ESC) {
        engine_cleanup(engine, EXIT_SUCCESS);
        return 1;
      }

      if (engine->player.health <= 0 || !allowGameplayInput)
        continue;

      if (sc == SDL_SCANCODE_E) {
        entities_tryInteract(engine);
      }

      // ungrab Mouse
      if (sc == UNGRAB_MOUSE && mouseUngrabbed == 0) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        mouseUngrabbed = 1;
      } else if (sc == UNGRAB_MOUSE &&
                 mouseUngrabbed == 1) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
        mouseUngrabbed = 0;
      }

      if (sc == CYCLE_GAME) {
        engine->mode = (engine->mode + 1) % TOTAL_MODES;
        switch (engine->mode) {
        case 0:
          printf("\033[35m[MODE] Current Game Mode: GAME\033[0m\n");
          break;
        case 1:
          printf("\033[35m[MODE] Current Game Mode: DEBUG\033[0m\n");
          break;
        }
      }

      // Reload
      /* if (event.key.keysym.scancode == GUN_RELOAD) { */
      /*   playShotgunReload(&engine->sound); */
      /* } */
    }

    /* MOUSE EVENTS */
    if (event.type == SDL_MOUSEWHEEL && !engine->player.shooting) {
      if (engine->player.health <= 0 || !allowGameplayInput)
        continue;
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
      if (engine->player.health <= 0 || !allowGameplayInput)
        continue;
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
      if (engine->player.health <= 0 || !allowGameplayInput)
        continue;
      if (event.button.button == MSB_LEFT) {
        engine->player.mouseHeld = 1;
        weaponProperties[engine->player.selectedGun].fireAccumulator = 0;

        if (weaponProperties[engine->player.selectedGun].ammunition > 0) {
          switch (engine->player.selectedGun) {
          case SHOTGUN:
            if (!animations.shotgun_shoot.playing) {
              weaponProperties[engine->player.selectedGun].ammunition--;
              animations.shotgun_shoot.playing = 1;
              playShotgunShot(&engine->sound);
              enemies_applyHitscanDamage(
                  engine, weaponProperties[engine->player.selectedGun].damage);
            }
            break;
          case ROCKET:
            if (!animations.rocket_shoot.playing) {
              weaponProperties[engine->player.selectedGun].ammunition--;
              animations.rocket_shoot.playing = 1;
              playRocketShot(&engine->sound);
              enemies_applyHitscanDamage(
                  engine, weaponProperties[engine->player.selectedGun].damage);
            }
            break;
          case PISTOL:
            if (!animations.pistol_shoot.playing) {
              animations.pistol_shoot.playing = 1;
              weaponProperties[engine->player.selectedGun].ammunition--;
              playPistolShot(&engine->sound);
              enemies_applyHitscanDamage(
                  engine, weaponProperties[engine->player.selectedGun].damage);
            }
            break;
          /* case HANDS: */
          /*   if (!animations.hands_punsh.playing) { */
          /*     animations.hands_punsh.playing = 1; */
          /*     playHandsPunsh(&engine->sound); */
          /*   } */
          /*   break; */
          case SINGLE:
            if (!animations.single_shoot.playing) {
              animations.single_shoot.playing = 1;
              weaponProperties[engine->player.selectedGun].ammunition--;
              playSingleShot(&engine->sound);
              enemies_applyHitscanDamage(
                  engine, weaponProperties[engine->player.selectedGun].damage);
            }
            break;
          case MINIGUN:
            if (!animations.minigun_shoot.playing) {
              animations.minigun_shoot.playing = 1;
              weaponProperties[engine->player.selectedGun].ammunition--;
              playMinigunShot(&engine->sound);
              enemies_applyHitscanDamage(
                  engine, weaponProperties[engine->player.selectedGun].damage);
            }
            break;
          default:
            break;
          }
        }
      }
    }

    if (event.type == SDL_MOUSEBUTTONUP) {
      if (event.button.button == MSB_LEFT) {
        engine->player.mouseHeld = 0;
      }
    }
  }

  if (!allowGameplayInput)
    engine->player.mouseHeld = 0;

  if (allowGameplayInput && engine->player.health > 0 && engine->player.mouseHeld &&
      weaponProperties[engine->player.selectedGun].ammunition > 0) {
    WeaponProperties *weapon = &weaponProperties[engine->player.selectedGun];
    if (weapon->automatic) {
      weapon->fireAccumulator += deltaTime;
      if (weapon->fireAccumulator >= weapon->fireRate) {
        weaponProperties[engine->player.selectedGun].ammunition--;
        weapon->fireAccumulator -= weapon->fireRate;
        switch (engine->player.selectedGun) {
        case MINIGUN:
          if (animations.minigun_shoot.playing) {
            playMinigunShot(&engine->sound);
          }
          break;
        default:
          break;
        }
        enemies_applyHitscanDamage(engine,
                                   weaponProperties[engine->player.selectedGun]
                                       .damage);
      }
    }
  } else {
    engine->player.shooting = 0;
    animations.minigun_shoot.playing = 0;
  }

  /* CONTINUOUS INPUT (held keys) */
  const Uint8 *state = SDL_GetKeyboardState(NULL);
  Sprite *sprites = engine->sprites;
  int spriteCount = entities_getSpriteCount();

  int moveDir = 0;
  if (allowGameplayInput && (state[SDL_SCANCODE_W] || state[SDL_SCANCODE_UP]))
    moveDir += 1;
  if (allowGameplayInput && (state[SDL_SCANCODE_S] || state[SDL_SCANCODE_DOWN]))
    moveDir -= 1;

  int strafeDir = 0;
  if (allowGameplayInput && state[SDL_SCANCODE_D])
    strafeDir += 1;
  if (allowGameplayInput && state[SDL_SCANCODE_A])
    strafeDir -= 1;

  if (engine->player.health > 0 && allowGameplayInput) {
    player_move(&engine->player, deltaTime, worldMap, sprites, spriteCount, moveDir);
    player_strafe(&engine->player, deltaTime, worldMap, sprites, spriteCount, strafeDir);
  } else {
    engine->player.velocityForward = 0.0;
    engine->player.velocityStrafe = 0.0;
    engine->player.velX = 0.0;
    engine->player.velY = 0.0;
    engine->player.mouseHeld = 0;
  }

  // Rotation with keys
  if (allowGameplayInput && state[SDL_SCANCODE_LEFT])
    player_rotate(&engine->player,
                  key_rotationAmount(engine->player.rotSpeed, deltaTime, 1));
  if (allowGameplayInput && state[SDL_SCANCODE_RIGHT])
    player_rotate(&engine->player,
                  key_rotationAmount(engine->player.rotSpeed, deltaTime, -1));

  double speed =
      sqrt(engine->player.velX * engine->player.velX +
           engine->player.velY * engine->player.velY);
  double normalized =
      (engine->player.moveSpeed > 0.0)
          ? fmin(speed / engine->player.moveSpeed, 1.0)
          : 0.0;
  if (normalized > 0.0)
  {
    engine->player.bobTime += deltaTime * (0.4 + 0.3 * normalized);
    if (engine->player.bobTime > 1000.0)
      engine->player.bobTime = fmod(engine->player.bobTime, 1000.0);
  }
  else
  {
    engine->player.bobTime = 0.0;
  }

  if (engine->player.damageFlashTimer > 0.0) {
    engine->player.damageFlashTimer -= deltaTime;
    if (engine->player.damageFlashTimer < 0.0)
      engine->player.damageFlashTimer = 0.0;
  }

  return 0;
}
