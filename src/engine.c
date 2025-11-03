#include "engine.h"
#include "entities.h"
#include "map.h"
#include "sound.h"
#include "weapons.h"
#include "types.h"
#include <stdio.h>

static const char *kLevelMapPaths[LEVEL_COUNT] = {
    "levels/1/map.csv",
    "levels/2/map.csv",
    "levels/3/map.csv",
    "levels/4/map.csv",
    "levels/5/map.csv",
};

static const char *kLevelEntitiesPaths[LEVEL_COUNT] = {
    "levels/1/entities.json",
    "levels/2/entities.json",
    "levels/3/entities.json",
    "levels/4/entities.json",
    "levels/5/entities.json",
};

int engine_loadLevel(Engine *engine, int levelIndex, bool resetPlayerState)
{
  if (!engine)
    return -1;
  if (levelIndex < 0 || levelIndex >= LEVEL_COUNT)
  {
    fprintf(stderr, "\033[31m[ERROR] Invalid level index %d\033[0m\n",
            levelIndex);
    return -1;
  }

  const char *mapPath = kLevelMapPaths[levelIndex];
  const char *entitiesPath = kLevelEntitiesPaths[levelIndex];

  if (map_loadFromCSV(mapPath) != 0)
  {
    fprintf(stderr,
            "\033[33m[WARN] Failed to load map '%s'; using built-in layout\033[0m\n",
            mapPath);
    map_resetToDefault();
  }

  entities_setEntitiesFilePath(entitiesPath);
  entities_reset();
  engine->sprites = entities_createWorldSprites();

  if (resetPlayerState)
  {
    weapons_resetProperties();
    player_respawn(&engine->player);
  }
  else
  {
    player_applySpawn(&engine->player);
    engine->player.velocityForward = 0.0;
    engine->player.velocityStrafe = 0.0;
    engine->player.velX = 0.0;
    engine->player.velY = 0.0;
    engine->player.damageFlashTimer = 0.0;
    engine->player.bobTime = 0.0;
    engine->player.mouseHeld = 0;
    engine->player.shooting = 0;
  }

  engine->currentLevelIndex = levelIndex;
  return 0;
}

int engine_init(Engine *engine) {

  engine->mode = GAME;
  engine->game = createGame();
  engine->currentLevelIndex = 0;

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
  loadAllAnimations();
  engine->font = font_init();

  // Initialize Time variables
  engine->time = SDL_GetTicks();
  engine->oldTime = 0;
  engine->deltaTime = 0;
  engine->fps = 0;
  engine->frameCount = 0;

  // Allocate buffers, load textures, animations
  buffers_init(&engine->game);
  textures_load(&engine->textures);
  loadSounds(&engine->sound);
  loadMusic(&engine->sound);

  if (engine_loadLevel(engine, 0, true) != 0)
    fprintf(stderr, "\033[33m[WARN] Failed to load initial level; using defaults\033[0m\n");

  // play background track (currently Soundtrack_intense)
  playTrackIntense(&engine->sound);

  for (int i = 0; i < NUM_TEXTURES; i++) {
    if (!engine->textures.textures[i]) {
      fprintf(stderr, "\033[31m[WARNING] textures[%d] is NULL!\033[0m\n", i);
    }
  }
  // FPS Mouse
  SDL_SetRelativeMouseMode(SDL_TRUE);

  return 0;
}

void engine_reloadLevel(Engine *engine)
{
  if (!engine)
    return;
  engine_loadLevel(engine, engine->currentLevelIndex, true);
}

void engine_updateTime(Engine *engine) {
  engine->oldTime = engine->time;
  engine->time = SDL_GetTicks();
  engine->deltaTime = (engine->time - engine->oldTime) / 1000.0;
  engine->fps = (engine->deltaTime > 0) ? (int)(1.0 / engine->deltaTime) : 0;
}

void engine_cleanup(Engine *engine, int exitCode) {
  printf("\033[32m[CLEANUP] Starting engine cleanup...\033[0m\n");

  freeAllAnimations();

  printf("\033[32m[CLEANUP] Freeing textures...\033[0m\n");
  for (int i = 0; i < NUM_TEXTURES; i++) {
    if (engine->textures.textures[i]) {
      free(engine->textures.textures[i]);
      engine->textures.textures[i] = NULL;
    }
  }

  printf("\033[32m[CLEANUP] Cleaning up sound...\033[0m\n");
  cleanupSound(&engine->sound);

  printf("\033[32m[CLEANUP] Closing fonts...\033[0m\n");
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

  printf("\033[32m[CLEANUP] Destroying SDL renderer, window, and "
         "texture...\033[0m\n");
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

  // Free buffes
  printf("\033[32m[CLEANUP] Freeing game buffers...\033[0m\n");
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

  printf("\033[32m[CLEANUP] Engine cleanup complete. Exiting.\033[0m\n");
  exit(exitCode);
}
