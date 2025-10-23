#include "input.h"

int handleInput(Engine *engine, double deltaTime) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {

    /* WINDOW */
    if (event.type == SDL_QUIT) {
      engine_cleanup(engine, EXIT_SUCCESS);
    }

    // resize
    if (event.type == SDL_WINDOWEVENT) {
      if (event.window.type == SDL_WINDOWEVENT_RESIZED) {
        engine->game.window_width = event.window.data1;
        engine->game.window_height = event.window.data2;

        buffers_reallocate(&engine->game);
      }
      if (engine->game.screen_texture) {
        SDL_DestroyTexture(engine->game.screen_texture);
      }
      engine->game.screen_texture = SDL_CreateTexture(
          engine->game.renderer, SDL_PIXELFORMAT_ARGB8888,
          SDL_TEXTUREACCESS_STREAMING, engine->game.window_width,
          engine->game.window_height);
      if (!engine->game.screen_texture) {
        fprintf(stderr, "Failed to recreate texture: %s\n", SDL_GetError());
        SDL_cleanup(&engine->game, EXIT_FAILURE);
        return 1;
      }

      // window key events
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.scancode == TOGGLE_FULLSCREEN) {
          static bool isFullscreen = false;
          isFullscreen = !isFullscreen;
          SDL_SetWindowFullscreen(engine->game.window,
                                  isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP
                                               : 0);
        }
        if (event.key.keysym.scancode == CLOSE_GAME_ESC) {
          engine_cleanup(engine, EXIT_SUCCESS);
        }
      }

      /* PLAYER */
      if (event.type == SDL_KEYDOWN) { // key pressed?
        if (event.key.keysym.scancode == GUN_RELOAD) {
          // TODO: Gun animation Logic, use helper function here->
          playShotgunReload(&engine->sound);
        }
      }

      // movement (key held?)
      const Uint8 *state = SDL_GetKeyboardState(NULL);
      if (state[SDL_SCANCODE_Q] && SDL_GetRelativeMouseMode())
        SDL_SetRelativeMouseMode(SDL_FALSE);
      else if (state[SDL_SCANCODE_Q] && !SDL_GetRelativeMouseMode())
        SDL_SetRelativeMouseMode(SDL_TRUE);
      if (state[SDL_SCANCODE_UP] || state[SDL_SCANCODE_W])
        player_move(&engine->player, deltaTime, worldMap, 1);
      if (state[SDL_SCANCODE_DOWN] || state[SDL_SCANCODE_S])
        player_move(&engine->player, deltaTime, worldMap, -1);
      if (state[SDL_SCANCODE_LEFT])
        player_rotate(
            &engine->player,
            key_rotationAmount(engine->player.rotSpeed, deltaTime, 1));
      if (state[SDL_SCANCODE_RIGHT])
        player_rotate(
            &engine->player,
            key_rotationAmount(engine->player.rotSpeed, deltaTime, -1));
      // strafing l+r
      if (state[SDL_SCANCODE_A])
        player_strafe(&engine->player, deltaTime, worldMap, -1);
      if (state[SDL_SCANCODE_D])
        player_strafe(&engine->player, deltaTime, worldMap, 1);

      // mouse control
      if (event.type == SDL_MOUSEMOTION) {
        player_rotate( // left and right
            &engine->player,
            mouse_rotationAmount(engine->player.sensX, -event.motion.xrel));
        // up and down (pitch)
        engine->player.pitch -=
            event.motion.yrel * engine->player.sensX * engine->player.sensY;
        if (engine->player.pitch > CLAMP)
          engine->player.pitch = CLAMP;
        else if (engine->player.pitch < -CLAMP)
          engine->player.pitch = -CLAMP;
      }

      // mouse clicks
      if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == MSB_LEFT && engine->gunAnim.playing == 0) {
          // TODO: switch gun animation so wrap this code in helper func:
          /* gunAnim.playing = 1; */
          /* gunAnim.currentFrame = 0; */
          /* gunAnim.timeAccumulator = 0.0; */
          /* gunAnim.currentAnim = gunTextures; */
          /* gunAnim.maxFrames = SHOTGUN_SHOOT_FRAMES; */
          playShotgunShot(&engine->sound);
        }
      }
    }
  }
  return 0;
}
