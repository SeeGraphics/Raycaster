#include "sound.h"
#include <stdio.h>

SoundManager createSound() {
  SoundManager s = {NULL, NULL};
  return s;
}

int initSound() {
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n",
           Mix_GetError());
    return -1;
  }
  return 0;
}

Mix_Chunk *loadSound(const char *path) {
  Mix_Chunk *sound = Mix_LoadWAV(path);
  if (!sound) {
    printf("Failed to load %s: %s\n", path, Mix_GetError());
  } else {
    printf("Loaded sound: %s\n", path);
  }
  return sound;
}

void loadSounds(SoundManager *soundManager) {
  soundManager->ShotgunShot = loadSound("assets/sounds/shotgun_shoot.mp3");
  soundManager->ShotgunReload = loadSound("assets/sounds/shotgun_reload.mp3");

  printf("Sounds loaded...\n");
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

void cleanupSound(SoundManager *soundManager) {
  if (soundManager->ShotgunShot) {
    Mix_FreeChunk(soundManager->ShotgunShot);
    soundManager->ShotgunShot = NULL;
  }
  Mix_CloseAudio();
}
