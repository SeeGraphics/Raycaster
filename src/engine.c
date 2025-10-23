#include "engine.h"
#include "gun.h"
#include "sound.h"

int engine_init(Engine *engine) {
  // SDL + TTF
  if (SDL_initialize(&engine->game))
    return 1;
  if (initSound() < 0)
    return 1;
  if (TTF_Init() == -1)
    return 1;

  // Initialize objects inside engine
  engine->game = createGame();
  engine->player = createPlayer();
  engine->textures = createTextures();
  engine->sound = createSound();
  engine->sprites = createSprite();
  engine->gunAnim = createGunAnim();
  engine->font = font_init();

  // Initialize Time variables
  engine->time = SDL_GetTicks();
  engine->oldTime = 0;
  engine->deltaTime = 0;
  engine->fps = 0;

  // Allocate buffers, load textures, load gun animations
  buffers_init(&engine->game);
  textures_load(&engine->textures);
  loadAllTextures_Shotgun_shoot(engine->game.renderer);
  loadAllTextures_Shotgun_reload(engine->game.renderer);
  loadSounds(&engine->sound);

  // FPS Mouse
  SDL_SetRelativeMouseMode(SDL_TRUE);

  return 0;
}

void engine_updateTime(Engine *engine) {
  engine->oldTime = engine->time;
  engine->time = SDL_GetTicks();
  engine->deltaTime = (engine->time - engine->oldTime) / 1000.0;
  engine->fps = (engine->deltaTime > 0) ? (int)(1.0 / engine->deltaTime) : 0;
}

void engine_cleanup(Engine *engine, int exitCode) {
  // Free game buffers
  if (engine->game.buffer) {
    free(engine->game.buffer);
    engine->game.buffer = NULL;
  }

  if (engine->game.Zbuffer) {
    free(engine->game.Zbuffer);
    engine->game.Zbuffer = NULL;
  }

  // Free textures
  for (int i = 0; i < NUM_TEXTURES; i++) {
    if (engine->textures.textures[i]) {
      free(engine->textures.textures[i]);
      engine->textures.textures[i] = NULL;
    }
  }

  // Free sprites if dynamically allocated
  if (engine->sprites) {
    free(engine->sprites);
    engine->sprites = NULL;
  }

  // Cleanup gun textures
  cleanupGunTextures();

  // Cleanup sound manager
  cleanupSound(&engine->sound);

  // Cleanup fonts
  if (engine->font.debug) {
    TTF_CloseFont(engine->font.debug);
    engine->font.debug = NULL;
  }
  if (engine->font.ui) {
    TTF_CloseFont(engine->font.ui);
    engine->font.ui = NULL;
  }
  if (engine->font.title) {
    TTF_CloseFont(engine->font.title);
    engine->font.title = NULL;
  }

  // Quit SDL
  SDL_cleanup(&engine->game, exitCode);
}
