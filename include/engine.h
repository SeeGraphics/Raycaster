#ifndef ENGINE_H
#define ENGINE_H

#include "animation.h"
#include "font.h"
#include "graphics.h"
#include "player.h"
#include "sound.h"
#include "sprites.h"
#include "texture.h"

typedef struct Engine {
  // Objects
  Game game;
  Player player;
  TextureManager textures;
  SoundManager sound;
  Sprite *sprites;
  Font font;

  // Time tracking
  double time, oldTime;
  double deltaTime;
  int fps;
  int frameCount;
} Engine;

int engine_init(Engine *engine);
void engine_updateTime(Engine *engine);
void engine_cleanup(Engine *engine, int exitCode);

#endif
