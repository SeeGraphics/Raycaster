#ifndef SOUND_H
#define SOUND_H

#include "SDL_mixer.h"

#define MUSIC_VOL 5
#define SFX_VOL 50

typedef struct {
  Mix_Chunk *ShotgunShot;
  Mix_Chunk *ShotgunReload;
  Mix_Chunk *RocketShot;
  Mix_Chunk *PistolShot;
  Mix_Chunk *HandsPunsh;
  Mix_Chunk *SingleShot;
  Mix_Chunk *MinigunShot;
  Mix_Music *Soundtrack_intense;
} SoundManager;

SoundManager createSound();
int initSound();
void loadSounds(SoundManager *soundManager);
void loadMusic(SoundManager *soundManager);

/* PLAY SOUNDS */
void playShotgunShot(SoundManager *soundManager);
void playShotgunReload(SoundManager *soundManager);
void playRocketShot(SoundManager *soundManager);
void playPistolShot(SoundManager *soundManager);
void playHandsPunsh(SoundManager *soundManager);
void playSingleShot(SoundManager *soundManager);
void playMinigunShot(SoundManager *soundManager);
void playTrackIntense(SoundManager *soundManager);

void cleanupSound(SoundManager *soundManager);

#endif
