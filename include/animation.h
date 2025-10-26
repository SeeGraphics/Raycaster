#ifndef ANIMATION_H
#define ANIMATION_H

#include "player.h"
#include "types.h"
#include <SDL.h>

#define FRAMETIME_SHOTGUN 0.12
#define FRAMETIME_ROCKET 0.09
#define FRAMETIME_PISTOL 0.09
#define FRAMETIME_HANDS 0.08

/* FRAME COUNTS */
#define FRAMES_SHOTGUN_SHOOT 3
#define FRAMES_ROCKET_SHOOT 5
#define FRAMES_PISTOL_SHOOT 6
#define FRAMES_HANDS_PUNSH 3

typedef struct {
  u32 *pixels;
  int width, height;
} Frame;

typedef struct {
  Frame *frames;
  int frameCount;
  double frameTime;
  double timeAccumulator;
  int currentFrame;
  int playing;
  int looping;
} Animation;

typedef struct {
  Animation shotgun_shoot;
  Animation shotgun_reload;
  Animation rocket_shoot;
  Animation pistol_shoot;
  Animation hands_punsh;
} AnimationRegistry;

extern AnimationRegistry animations;

void loadAllAnimations();
void updateAllAnimations(Player *player, double deltaTime);
void blitAnimation(u32 *buffer, Animation *animation, float width, float height,
                   float x, float y, float scale);
void freeAllAnimations();

#endif
