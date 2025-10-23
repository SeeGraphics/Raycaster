#ifndef ANIMATION_H
#define ANIMATION_H

#include <SDL.h>

typedef struct {
  double frameTime;
  int playing;
  int numberFrames;
} Animation;

Animation createAnimation();
SDL_Texture *loadTexture();

#endif
