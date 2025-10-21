#ifndef SOUND_H
#define SOUND_H

#include "SDL_mixer.h"

typedef struct {
  Mix_Chunk *gunShot;
} SoundManager;

int initSound();
void loadSounds(SoundManager *soundManager);
void playGunShot(SoundManager *soundManager);
void cleanupSound(SoundManager *soundManager);

#endif
