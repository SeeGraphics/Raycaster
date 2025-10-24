#ifndef SOUND_H
#define SOUND_H

#include "SDL_mixer.h"

#define MUSIC_VOL 10
#define SFX_VOL 50

typedef struct {
  Mix_Chunk *ShotgunShot;
  Mix_Chunk *ShotgunReload;
  Mix_Music *Soundtrack_intense;
} SoundManager;

SoundManager createSound();
int initSound();
void loadSounds(SoundManager *soundManager);
void loadMusic(SoundManager *soundManager);
void playShotgunShot(SoundManager *soundManager);
void playShotgunReload(SoundManager *soundManager);
void playTrackIntense(SoundManager *soundManager);
void cleanupSound(SoundManager *soundManager);

#endif
