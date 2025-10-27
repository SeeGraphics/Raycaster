#include "sound.h"
#include <stdio.h>

SoundManager createSound() {
  SoundManager s = {NULL, NULL, NULL, NULL};
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
    fprintf(stderr, "[SOUND] Loaded sound: %s\n", path);
  }
  return sound;
}

Mix_Music *loadTrack(const char *path) {
  Mix_Music *track = Mix_LoadMUS(path);
  if (!track) {
    fprintf(stderr, "[ERROR] Failed to track %s: %s\n", path, Mix_GetError());
  } else {
    fprintf(stderr, "[SOUND] Loaded track: %s\n", path);
  }
  return track;
}

void loadSounds(SoundManager *soundManager) {
  soundManager->ShotgunShot =
      loadSound("assets/sounds/weapons/shotgun/shotgun_shoot.mp3");
  soundManager->ShotgunReload =
      loadSound("assets/sounds/weapons/shotgun/shotgun_reload.mp3");
  soundManager->RocketShot =
      loadSound("assets/sounds/weapons/rocket/rocket_shoot.mp3");
  soundManager->PistolShot =
      loadSound("assets/sounds/weapons/pistol/pistol_shoot.mp3");
  soundManager->HandsPunsh = loadSound("assets/sounds/weapons/hands/punsh.wav");
  soundManager->SingleShot =
      loadSound("assets/sounds/weapons/shotgun/single_shoot.wav");
  soundManager->MinigunShot =
      loadSound("assets/sounds/weapons/minigun/minigun_shoot.wav");

  Mix_Volume(-1, SFX_VOL);
  printf("[SOUND] Sounds loaded...\n");
}

void loadMusic(SoundManager *soundManager) {
  soundManager->Soundtrack_intense =
      loadTrack("assets/sounds/music/Soundtrack_intense.mp3");

  Mix_VolumeMusic(MUSIC_VOL);
  printf("[SOUND] Tracks loaded...\n");
}

/* PLAY SOUNDS *************************************************************/
void playShotgunShot(SoundManager *soundManager) {
  if (soundManager->ShotgunShot) {
    Mix_PlayChannel(-1, soundManager->ShotgunShot, 0);
  }
}
void playSingleShot(SoundManager *soundManager) {
  if (soundManager->SingleShot) {
    Mix_PlayChannel(-1, soundManager->SingleShot, 0);
  }
}
void playShotgunReload(SoundManager *soundManager) {
  if (soundManager->ShotgunReload) {
    Mix_PlayChannel(-1, soundManager->ShotgunReload, 0);
  }
}
void playRocketShot(SoundManager *soundManager) {
  if (soundManager->RocketShot) {
    Mix_PlayChannel(-1, soundManager->RocketShot, 0);
  }
}
void playPistolShot(SoundManager *soundManager) {
  if (soundManager->PistolShot) {
    Mix_PlayChannel(-1, soundManager->PistolShot, 0);
  }
}
void playMinigunShot(SoundManager *soundManager) {
  if (soundManager->MinigunShot) {
    Mix_PlayChannel(-1, soundManager->MinigunShot, 0);
  }
}
void playHandsPunsh(SoundManager *soundManager) {
  if (soundManager->HandsPunsh) {
    Mix_PlayChannel(-1, soundManager->HandsPunsh, 0);
  }
}
void playTrackIntense(SoundManager *soundManager) {
  if (soundManager->Soundtrack_intense) {
    Mix_PlayMusic(soundManager->Soundtrack_intense, -1);
  }
}
/****************************************************************************/

/* CLEANUP */
void cleanupSound(SoundManager *soundManager) {
  if (soundManager->ShotgunShot) {
    Mix_FreeChunk(soundManager->ShotgunShot);
    soundManager->ShotgunShot = NULL;
  }
  Mix_CloseAudio();
}
