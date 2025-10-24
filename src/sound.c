#include "sound.h"
#include <stdio.h>

SoundManager createSound() {
  SoundManager s = {NULL, NULL, NULL};
  return s;
}

int initSound() {
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    printf("[ERROR] SDL_mixer could not initialize! SDL_mixer Error: %s\n",
           Mix_GetError());
    return -1;
  }
  return 0;
}

Mix_Chunk *loadSound(const char *path) {
  Mix_Chunk *sound = Mix_LoadWAV(path);
  if (!sound) {
    fprintf(stderr, "[ERROR] Failed to load %s: %s\n", path, Mix_GetError());
  } else {
    fprintf(stderr, "[ERROR] Loaded sound: %s\n", path);
  }
  return sound;
}

Mix_Music *loadTrack(const char *path) {
  Mix_Music *track = Mix_LoadMUS(path);
  if (!track) {
    fprintf(stderr, "[ERROR] Failed to track %s: %s\n", path, Mix_GetError());
  } else {
    fprintf(stderr, "[ERROR] Loaded track: %s\n", path);
  }
  return track;
}

void loadSounds(SoundManager *soundManager) {
  soundManager->ShotgunShot = loadSound("assets/sounds/shotgun_shoot.mp3");
  soundManager->ShotgunReload = loadSound("assets/sounds/shotgun_reload.mp3");

  Mix_Volume(-1, SFX_VOL);
  printf("[SOUND] Sounds loaded...\n");
}

void loadMusic(SoundManager *soundManager) {
  soundManager->Soundtrack_intense =
      loadTrack("assets/sounds/Soundtrack_intense.mp3");

  Mix_VolumeMusic(MUSIC_VOL);
  printf("[SOUND] Tracks loaded...\n");
}

void playShotgunShot(SoundManager *soundManager) {
  if (soundManager->ShotgunShot) {
    Mix_PlayChannel(-1, soundManager->ShotgunShot, 0);
  }
}

void playShotgunReload(SoundManager *soundManager) {
  if (soundManager->ShotgunReload) {
    Mix_PlayChannel(-1, soundManager->ShotgunReload, 0);
  }
}

void playTrackIntense(SoundManager *soundManager) {
  if (soundManager->Soundtrack_intense) {
    Mix_PlayMusic(soundManager->Soundtrack_intense, -1);
  }
}

void cleanupSound(SoundManager *soundManager) {
  if (soundManager->ShotgunShot) {
    Mix_FreeChunk(soundManager->ShotgunShot);
    soundManager->ShotgunShot = NULL;
  }
  Mix_CloseAudio();
}
