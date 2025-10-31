#include "engine.h"
#include "entities.h"
#include "map.h"
#include "sound.h"
#include <math.h>
#include <stdio.h>

static void engine_applyPlayerSpawn(Player *player)
{
  if (!player)
    return;

  double spawnX = player->posX;
  double spawnY = player->posY;
  double spawnDirDegrees = 0.0;
  entities_getPlayerSpawn(&spawnX, &spawnY, &spawnDirDegrees);

  player->posX = spawnX;
  player->posY = spawnY;

  const double degToRad = 3.14159265358979323846 / 180.0;
  double dirRad = spawnDirDegrees * degToRad;
  player->dirX = cos(dirRad);
  player->dirY = -sin(dirRad);

  double planeScale = hypot(PLANE_X, PLANE_Y);
  if (planeScale <= 0.0)
    planeScale = 0.88;
  player->planeX = player->dirY * planeScale;
  player->planeY = -player->dirX * planeScale;
}

int engine_init(Engine *engine) {

  engine->mode = GAME;
  engine->game = createGame();

  if (map_loadFromCSV("levels/1/map.csv") != 0)
    fprintf(stderr, "\033[33m[WARN] Using built-in map layout\033[0m\n");

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
  engine->sprites = entities_createWorldSprites();
  engine->font = font_init();
  engine_applyPlayerSpawn(&engine->player);

  // Initialize Time variables
  engine->time = SDL_GetTicks();
  engine->oldTime = 0;
  engine->deltaTime = 0;
  engine->fps = 0;
  engine->frameCount = 0;

  // Allocate buffers, load textures, animations
  buffers_init(&engine->game);
  loadAllAnimations();
  textures_load(&engine->textures);
  loadSounds(&engine->sound);
  loadMusic(&engine->sound);

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
