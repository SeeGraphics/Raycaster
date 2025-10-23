#ifndef SOUND_H
#define SOUND_H

#include "SDL_mixer.h"

typedef struct {
  Mix_Chunk *ShotgunShot;
  Mix_Chunk *ShotgunReload;
} SoundManager;

SoundManager createSound();
int initSound();
void loadSounds(SoundManager *soundManager);
void playShotgunShot(SoundManager *soundManager);
void playShotgunReload(SoundManager *soundManager);
void cleanupSound(SoundManager *soundManager);

#endif
