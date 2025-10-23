#ifndef GUN_H
#define GUN_H

#include "graphics.h"

#define SHOTGUN_SHOOT_FRAMES 8
#define SHOTGUN_RELOAD_FRAMES 9

extern SDL_Texture *gunTextures[SHOTGUN_SHOOT_FRAMES];
extern SDL_Texture *reloadArrays[SHOTGUN_RELOAD_FRAMES];

typedef struct {
  int currentFrame;
  double frameTime;
  double timeAccumulator;
  int playing;
  SDL_Texture **currentAnim;
  int maxFrames;
} GunAnim;

// init
GunAnim createGunAnim();

SDL_Texture *loadGunTexture(SDL_Renderer *renderer, const char *filePath);
void loadAllTextures_Shotgun_shoot(SDL_Renderer *renderer);
void loadAllTextures_Shotgun_reload(SDL_Renderer *renderer);
void drawGunTexture(Game *game, SDL_Texture *gunTexture, float widthFactor,
                    float heightFactor);
void cleanupGunTextures();

#endif
