#include "engine.h"
#include "blood.h"
#include "animation.h"
#include "entities.h"
#include "enemies.h"
#include "lights.h"
#include "map.h"
#include "render.h"
#include "sound.h"
#include "weapons.h"
#include "types.h"
#include <SDL.h>
#include <math.h>
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

static const double TRANSITION_CLOSE_TIME = 0.45;
static const double TRANSITION_HOLD_TIME = 0.10;
static const double TRANSITION_OPEN_TIME = 0.45;

static void engine_updateKeyPickup(Engine *engine, double deltaTime)
{
  if (!engine)
    return;

  if (engine->keyPickupTimer <= 0.0)
  {
    engine->keyPickupTimer = 0.0;
    engine->keyPickupOpacity = 0.0;
    return;
  }

  engine->keyPickupTimer -= deltaTime;
  if (engine->keyPickupTimer < 0.0)
    engine->keyPickupTimer = 0.0;

  const double fadeDuration = 1.5;
  if (engine->keyPickupTimer > fadeDuration)
  {
    engine->keyPickupOpacity = 1.0;
  }
  else
  {
    double t = engine->keyPickupTimer / fadeDuration;
    if (t < 0.0)
      t = 0.0;
    if (t > 1.0)
      t = 1.0;
    engine->keyPickupOpacity = t;
  }
}

static void engine_updateLevelBanner(Engine *engine, double deltaTime)
{
  if (!engine)
    return;

  if (engine->levelBannerTimer <= 0.0)
  {
    engine->levelBannerTimer = 0.0;
    engine->levelBannerOpacity = 0.0;
    return;
  }

  engine->levelBannerTimer -= deltaTime;
  if (engine->levelBannerTimer < 0.0)
    engine->levelBannerTimer = 0.0;

  const double fadeDuration = 1.5;
  if (engine->levelBannerTimer > fadeDuration)
  {
    engine->levelBannerOpacity = 1.0;
  }
  else
  {
    double t = engine->levelBannerTimer / fadeDuration;
    if (t < 0.0)
      t = 0.0;
    if (t > 1.0)
      t = 1.0;
    engine->levelBannerOpacity = t;
  }
}

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
  blood_reset();
  engine->sprites = entities_createWorldSprites();
  lights_buildMap();
  engine->keyPickupTimer = 0.0;
  engine->keyPickupOpacity = 0.0;
  engine->levelBannerTimer = 3.5;
  engine->levelBannerOpacity = 1.0;

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

void engine_startLevelTransition(Engine *engine, int levelIndex, bool resetPlayerState)
{
  if (!engine)
    return;
  if (levelIndex < 0 || levelIndex >= LEVEL_COUNT)
  {
    engine_loadLevel(engine, levelIndex, resetPlayerState);
    return;
  }
  if (engine->transitionActive)
    return;

  engine->transitionActive = true;
  engine->transitionPhase = 0;
  engine->transitionTimer = 0.0;
  engine->transitionRadius = 1.0;
  engine->transitionTargetLevel = levelIndex;
  engine->transitionResetPlayer = resetPlayerState;
  engine->transitionLevelLoaded = false;
  engine->player.mouseHeld = 0;
  engine->player.shooting = 0;
}

bool engine_isTransitionActive(const Engine *engine)
{
  return engine && engine->transitionActive;
}

void engine_updateTransition(Engine *engine, double deltaTime)
{
  if (!engine || !engine->transitionActive)
    return;

  if (deltaTime < 0.0)
    deltaTime = 0.0;

  switch (engine->transitionPhase)
  {
  case 0: // closing
  {
    engine->transitionTimer += deltaTime;
    double progress = engine->transitionTimer / TRANSITION_CLOSE_TIME;
    if (progress >= 1.0)
    {
      engine->transitionRadius = 0.0;
      if (!engine->transitionLevelLoaded)
      {
        if (engine_loadLevel(engine, engine->transitionTargetLevel,
                             engine->transitionResetPlayer) == 0)
          engine->transitionLevelLoaded = true;
      }
      engine->transitionPhase = 1;
      engine->transitionTimer = 0.0;
    }
    else
    {
      engine->transitionRadius = 1.0 - progress;
      if (engine->transitionRadius < 0.0)
        engine->transitionRadius = 0.0;
    }
    break;
  }
  case 1: // hold (fully closed)
    if (!engine->transitionLevelLoaded)
    {
      if (engine_loadLevel(engine, engine->transitionTargetLevel,
                           engine->transitionResetPlayer) == 0)
        engine->transitionLevelLoaded = true;
    }
    engine->transitionRadius = 0.0;
    engine->transitionTimer += deltaTime;
    if (engine->transitionTimer >= TRANSITION_HOLD_TIME)
    {
      engine->transitionPhase = 2;
      engine->transitionTimer = 0.0;
    }
    break;
  case 2: // opening
  {
    engine->transitionTimer += deltaTime;
    double progress = engine->transitionTimer / TRANSITION_OPEN_TIME;
    if (progress >= 1.0)
    {
      engine->transitionRadius = 1.0;
      engine->transitionActive = false;
      engine->transitionPhase = 0;
      engine->transitionTimer = 0.0;
      engine->transitionLevelLoaded = false;
    }
    else
    {
      engine->transitionRadius = progress;
      if (engine->transitionRadius > 1.0)
        engine->transitionRadius = 1.0;
    }
    break;
  }
  default:
    engine->transitionActive = false;
    engine->transitionRadius = 1.0;
    engine->transitionPhase = 0;
    engine->transitionTimer = 0.0;
    break;
  }
}

void engine_applyTransitionOverlay(Engine *engine)
{
  if (!engine || !engine->transitionActive)
    return;
  if (!engine->game.Rbuffer)
    return;

  double radiusNorm = engine->transitionRadius;
  if (radiusNorm >= 0.9999)
    return;

  const int width = RENDER_WIDTH;
  const int height = RENDER_HEIGHT;
  u32 *buffer = engine->game.Rbuffer;

  if (radiusNorm <= 0.0001)
  {
    for (int i = 0; i < width * height; ++i)
      buffer[i] = 0xFF000000;
    return;
  }

  double centerX = (double)width * 0.5;
  double centerY = (double)height * 0.5;
  double maxRadius = hypot(centerX, centerY);
  double currentRadius = radiusNorm * maxRadius;
  double radiusSq = currentRadius * currentRadius;

  for (int y = 0; y < height; ++y)
  {
    double dy = ((double)y + 0.5) - centerY;
    double dySq = dy * dy;
    int rowOffset = y * width;
    for (int x = 0; x < width; ++x)
    {
      double dx = ((double)x + 0.5) - centerX;
      double distSq = dx * dx + dySq;
      if (distSq > radiusSq)
        buffer[rowOffset + x] = 0xFF000000;
    }
  }
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

  engine->transitionActive = false;
  engine->transitionPhase = 0;
  engine->transitionTimer = 0.0;
  engine->transitionRadius = 1.0;
  engine->transitionTargetLevel = 0;
  engine->transitionResetPlayer = true;
  engine->transitionLevelLoaded = false;
  engine->keyPickupTimer = 0.0;
  engine->keyPickupOpacity = 0.0;

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
  engine->transitionActive = false;
  engine->transitionPhase = 0;
  engine->transitionTimer = 0.0;
  engine->transitionRadius = 1.0;
  engine->transitionLevelLoaded = false;
  engine->keyPickupTimer = 0.0;
  engine->keyPickupOpacity = 0.0;
  engine->levelBannerTimer = 0.0;
  engine->levelBannerOpacity = 0.0;
}

void engine_updateTime(Engine *engine) {
  engine->oldTime = engine->time;
  engine->time = SDL_GetTicks();
  engine->deltaTime = (engine->time - engine->oldTime) / 1000.0;
  engine->fps = (engine->deltaTime > 0) ? (int)(1.0 / engine->deltaTime) : 0;
}

void engine_frame(Engine *engine)
{
  if (!engine)
    return;

  engine_updateTransition(engine, engine->deltaTime);
  blood_update(engine->deltaTime);
  engine_updateKeyPickup(engine, engine->deltaTime);
  engine_updateLevelBanner(engine, engine->deltaTime);
  entities_handlePickups(engine);
  enemies_update(engine, engine->deltaTime);
  updateAllAnimations(&engine->player, engine->deltaTime);
  drawScene(engine);
  SDL_RenderPresent(engine->game.renderer);
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
