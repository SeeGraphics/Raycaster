#ifndef ENGINE_H
#define ENGINE_H

#include "font.h"
#include "graphics.h"
#include "gun.h"
#include "player.h"
#include "sound.h"
#include "sprites.h"
#include "texture.h"

typedef struct {
  // Objects
  Game game;
  Player player;
  TextureManager textures;
  SoundManager sound;
  Sprite *sprites;
  GunAnim gunAnim;
  Font font;

  // Time tracking
  double time, oldTime;
  double deltaTime;
  int fps;
} Engine;

int engine_init(Engine *engine);
void engine_updateTime(Engine *engine);
void engine_cleanup(Engine *engine, int exitCode);

#endif
