#include "engine.h"
#include "sound.h"

int engine_init(Engine *engine) {

  engine->game = createGame();

  // SDL + TTF
  if (SDL_initialize(&engine->game))
    return 1;
  if (initSound() < 0)
    return 1;
  if (TTF_Init() == -1)
    return 1;

  // Initialize objects inside engine
  engine->player = createPlayer();
  engine->textures = createTextures();
  engine->sound = createSound();
  engine->sprites = createSprite();
  engine->animation = createAnimation();
  engine->font = font_init();

  // Initialize Time variables
  engine->time = SDL_GetTicks();
  engine->oldTime = 0;
  engine->deltaTime = 0;
  engine->fps = 0;

  // Allocate buffers, load textures, animations
  buffers_init(&engine->game);
  textures_load(&engine->textures);
  loadSounds(&engine->sound);
  loadMusic(&engine->sound);
  loadAllAnimations(engine->game.renderer);

  // play background track (currently Soundtrack_intense)
  playTrackIntense(&engine->sound);

  for (int i = 0; i < NUM_TEXTURES; i++) {
    if (!engine->textures.textures[i]) {
      fprintf(stderr, "[WARNING] textures[%d] is NULL!\n", i);
    }
  }
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
  printf("[CLEANUP] Starting engine cleanup...\n");

  // 1️⃣ Cleanup animation textures first (they rely on the renderer)
  printf("[CLEANUP] Destroying animations...\n");
  cleanupAnimations();

  // 2️⃣ Free engine textures
  printf("[CLEANUP] Freeing textures...\n");
  for (int i = 0; i < NUM_TEXTURES; i++) {
    if (engine->textures.textures[i]) {
      free(engine->textures.textures[i]);
      engine->textures.textures[i] = NULL;
    }
  }

  // 3️⃣ Cleanup sound manager
  printf("[CLEANUP] Cleaning up sound...\n");
  cleanupSound(&engine->sound);

  // 4️⃣ Cleanup fonts
  printf("[CLEANUP] Closing fonts...\n");
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

  // 5️⃣ Destroy SDL objects last
  printf("[CLEANUP] Destroying SDL renderer, window, and texture...\n");
  if (engine->game.renderer) {
    SDL_DestroyRenderer(engine->game.renderer);
    engine->game.renderer = NULL;
  }
  if (engine->game.screen_texture) {
    SDL_DestroyTexture(engine->game.screen_texture);
    engine->game.screen_texture = NULL;
  }
  if (engine->game.window) {
    SDL_DestroyWindow(engine->game.window);
    engine->game.window = NULL;
  }

  SDL_Quit();

  // 6️⃣ Free buffes
  printf("[CLEANUP] Freeing game buffers...\n");
  if (engine->game.buffer) {
    free(engine->game.buffer);
    engine->game.buffer = NULL;
  }
  if (engine->game.Rbuffer) {
    free(engine->game.Rbuffer);
    engine->game.Rbuffer = NULL;
  }
  if (engine->game.Zbuffer) {
    free(engine->game.Zbuffer);
    engine->game.Zbuffer = NULL;
  }

  printf("[CLEANUP] Engine cleanup complete. Exiting.\n");
  exit(exitCode);
}
