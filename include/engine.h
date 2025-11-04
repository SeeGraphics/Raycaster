#ifndef ENGINE_H
#define ENGINE_H

#include "animation.h"
#include "font.h"
#include "graphics.h"
#include "player.h"
#include "sound.h"
#include "sprites.h"
#include "texture.h"
#include <stdbool.h>

typedef enum { GAME, DEBUG, TOTAL_MODES } GameMode;

typedef struct Engine {
  // Mode
  int mode;

  // Objects
  Game game;
  Player player;
  TextureManager textures;
  SoundManager sound;
  Sprite *sprites;
  Font font;
  int currentLevelIndex;
  bool transitionActive;
  int transitionPhase;
  double transitionTimer;
  double transitionRadius;
  int transitionTargetLevel;
  bool transitionResetPlayer;
  bool transitionLevelLoaded;
  double keyPickupTimer;
  double keyPickupOpacity;
  double levelBannerTimer;
  double levelBannerOpacity;

  // Time tracking
  double time, oldTime;
  double deltaTime;
  int fps;
  int frameCount;
} Engine;

int engine_init(Engine *engine);
void engine_reloadLevel(Engine *engine);
int engine_loadLevel(Engine *engine, int levelIndex, bool resetPlayerState);
void engine_startLevelTransition(Engine *engine, int levelIndex, bool resetPlayerState);
void engine_updateTransition(Engine *engine, double deltaTime);
void engine_applyTransitionOverlay(Engine *engine);
bool engine_isTransitionActive(const Engine *engine);
void engine_updateTime(Engine *engine);
void engine_frame(Engine *engine);
void engine_cleanup(Engine *engine, int exitCode);

#endif
