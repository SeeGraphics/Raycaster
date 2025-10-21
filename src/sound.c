#include "sound.h"
#include <stdio.h>

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
  soundManager->gunShot = loadSound("assets/sounds/shotgun_shoot.mp3");

  printf("Sounds loaded...\n");
}

void playGunShot(SoundManager *soundManager) {
  if (soundManager->gunShot) {
    Mix_PlayChannel(-1, soundManager->gunShot, 0);
  }
}

void cleanupSound(SoundManager *soundManager) {
  if (soundManager->gunShot) {
    Mix_FreeChunk(soundManager->gunShot);
    soundManager->gunShot = NULL;
  }
  Mix_CloseAudio();
}
