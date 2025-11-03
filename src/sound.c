#include "sound.h"
#include <stdio.h>
#include <string.h>

static const int VOLUME_GLOBAL = SFX_VOL;
static const int VOLUME_GUNS = 40;
static const int VOLUME_PICKUPS = 200;
static const int VOLUME_DEMON_DEATH = 90;

SoundManager createSound()
{
  SoundManager s;
  memset(&s, 0, sizeof(SoundManager));
  return s;
}

int initSound()
{
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
  {
    printf("\033[31m[ERROR] SDL_mixer could not initialize! SDL_mixer Error: "
           "%s\033[0m\n",
           Mix_GetError());
    return -1;
  }
  return 0;
}

Mix_Chunk *loadSound(const char *path)
{
  Mix_Chunk *sound = Mix_LoadWAV(path);
  if (!sound)
  {
    fprintf(stderr, "\033[31m[ERROR] Failed to load %s: %s\033[0m\n", path,
            Mix_GetError());
  }
  else
  {
    fprintf(stderr, "\033[32m[SOUND] Loaded sound: %s\033[0m\n", path);
  }
  return sound;
}

Mix_Music *loadTrack(const char *path)
{
  Mix_Music *track = Mix_LoadMUS(path);
  if (!track)
  {
    fprintf(stderr, "\033[31m[ERROR] Failed to track %s: %s\033[0m\n", path,
            Mix_GetError());
  }
  else
  {
    fprintf(stderr, "\033[32m[SOUND] Loaded track: %s\033[0m\n", path);
  }
  return track;
}

void loadSounds(SoundManager *soundManager)
{
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
  soundManager->PlayerHit =
      loadSound("assets/sounds/player/player_hit.wav");
  soundManager->PlayerDeath =
      loadSound("assets/sounds/player/player_death.wav");
  soundManager->PlayerHeal =
      loadSound("assets/sounds/player/player_heal.wav");
  soundManager->PlayerAmmo =
      loadSound("assets/sounds/player/player_ammo.ogg");
  soundManager->PlayerKey =
      loadSound("assets/sounds/player/player_key.wav");
  soundManager->DemonDeath =
      loadSound("assets/sounds/enemies/demon_die.wav");
  soundManager->DoorOpen =
      loadSound("assets/sounds/environment/door_open.wav");
  soundManager->DoorClose =
      loadSound("assets/sounds/environment/door_close.wav");

  Mix_Volume(-1, VOLUME_GLOBAL);
#define SET_VOL(chunk, vol)          \
  do                                 \
  {                                  \
    if (chunk)                       \
      Mix_VolumeChunk(chunk, (vol)); \
  } while (0)
  SET_VOL(soundManager->ShotgunShot, VOLUME_GUNS);
  SET_VOL(soundManager->ShotgunReload, VOLUME_GUNS);
  SET_VOL(soundManager->RocketShot, VOLUME_GUNS);
  SET_VOL(soundManager->PistolShot, VOLUME_GUNS);
  SET_VOL(soundManager->HandsPunsh, VOLUME_GUNS);
  SET_VOL(soundManager->SingleShot, VOLUME_GUNS);
  SET_VOL(soundManager->MinigunShot, VOLUME_GUNS);

  SET_VOL(soundManager->PlayerHit, VOLUME_GUNS);
  SET_VOL(soundManager->PlayerDeath, VOLUME_GUNS);
  SET_VOL(soundManager->PlayerHeal, VOLUME_PICKUPS);
  SET_VOL(soundManager->PlayerAmmo, VOLUME_PICKUPS);
  SET_VOL(soundManager->PlayerKey, VOLUME_PICKUPS);
  SET_VOL(soundManager->DemonDeath, VOLUME_DEMON_DEATH);
  SET_VOL(soundManager->DoorOpen, VOLUME_PICKUPS);
  SET_VOL(soundManager->DoorClose, VOLUME_PICKUPS);
#undef SET_VOL

  printf("\033[32m[SOUND] Sounds loaded...\033[0m\n");
}

void loadMusic(SoundManager *soundManager)
{
  soundManager->Soundtrack_intense =
      loadTrack("assets/sounds/music/Soundtrack_intense.mp3");

  Mix_VolumeMusic(MUSIC_VOL);
  printf("\033[32m[SOUND] Tracks loaded...\033[0m\n");
}

/* PLAY SOUNDS *************************************************************/
void playShotgunShot(SoundManager *soundManager)
{
  if (soundManager->ShotgunShot)
  {
    Mix_PlayChannel(-1, soundManager->ShotgunShot, 0);
  }
}
void playSingleShot(SoundManager *soundManager)
{
  if (soundManager->SingleShot)
  {
    Mix_PlayChannel(-1, soundManager->SingleShot, 0);
  }
}
void playShotgunReload(SoundManager *soundManager)
{
  if (soundManager->ShotgunReload)
  {
    Mix_PlayChannel(-1, soundManager->ShotgunReload, 0);
  }
}
void playRocketShot(SoundManager *soundManager)
{
  if (soundManager->RocketShot)
  {
    Mix_PlayChannel(-1, soundManager->RocketShot, 0);
  }
}
void playPistolShot(SoundManager *soundManager)
{
  if (soundManager->PistolShot)
  {
    Mix_PlayChannel(-1, soundManager->PistolShot, 0);
  }
}
void playMinigunShot(SoundManager *soundManager)
{
  if (soundManager->MinigunShot)
  {
    Mix_PlayChannel(-1, soundManager->MinigunShot, 0);
  }
}
void playHandsPunsh(SoundManager *soundManager)
{
  if (soundManager->HandsPunsh)
  {
    Mix_PlayChannel(-1, soundManager->HandsPunsh, 0);
  }
}
void playTrackIntense(SoundManager *soundManager)
{
  if (soundManager->Soundtrack_intense)
  {
    Mix_PlayMusic(soundManager->Soundtrack_intense, -1);
  }
}
void playPlayerHit(SoundManager *soundManager)
{
  if (soundManager->PlayerHit)
  {
    Mix_PlayChannel(-1, soundManager->PlayerHit, 0);
  }
}
void playPlayerDeath(SoundManager *soundManager)
{
  if (soundManager->PlayerDeath)
  {
    Mix_PlayChannel(-1, soundManager->PlayerDeath, 0);
  }
}
void playPlayerHeal(SoundManager *soundManager)
{
  if (soundManager->PlayerHeal)
  {
    Mix_PlayChannel(-1, soundManager->PlayerHeal, 0);
  }
}
void playPlayerAmmo(SoundManager *soundManager)
{
  if (soundManager->PlayerAmmo)
  {
    Mix_PlayChannel(-1, soundManager->PlayerAmmo, 0);
  }
}
void playPlayerKey(SoundManager *soundManager)
{
  if (soundManager->PlayerKey)
  {
    Mix_PlayChannel(-1, soundManager->PlayerKey, 0);
  }
}
void playDoorOpen(SoundManager *soundManager)
{
  if (soundManager->DoorOpen)
  {
    Mix_PlayChannel(-1, soundManager->DoorOpen, 0);
  }
}
void playDoorClose(SoundManager *soundManager)
{
  if (soundManager->DoorClose)
  {
    Mix_PlayChannel(-1, soundManager->DoorClose, 0);
  }
}
void playDemonDeath(SoundManager *soundManager)
{
  if (soundManager->DemonDeath)
  {
    Mix_PlayChannel(-1, soundManager->DemonDeath, 0);
  }
}
/****************************************************************************/

/* CLEANUP */
void cleanupSound(SoundManager *soundManager)
{
  if (!soundManager)
    return;
#define FREE_CHUNK(field)                 \
  do                                      \
  {                                       \
    if (soundManager->field)              \
    {                                     \
      Mix_FreeChunk(soundManager->field); \
      soundManager->field = NULL;         \
    }                                     \
  } while (0)

  FREE_CHUNK(ShotgunShot);
  FREE_CHUNK(ShotgunReload);
  FREE_CHUNK(RocketShot);
  FREE_CHUNK(PistolShot);
  FREE_CHUNK(HandsPunsh);
  FREE_CHUNK(SingleShot);
  FREE_CHUNK(MinigunShot);
  FREE_CHUNK(PlayerHit);
  FREE_CHUNK(PlayerDeath);
  FREE_CHUNK(PlayerHeal);
  FREE_CHUNK(PlayerAmmo);
  FREE_CHUNK(PlayerKey);
  FREE_CHUNK(DemonDeath);
  FREE_CHUNK(DoorOpen);
  FREE_CHUNK(DoorClose);
#undef FREE_CHUNK

  if (soundManager->Soundtrack_intense)
  {
    Mix_FreeMusic(soundManager->Soundtrack_intense);
    soundManager->Soundtrack_intense = NULL;
  }

  Mix_CloseAudio();
}
