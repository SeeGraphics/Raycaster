#ifndef GUN_H
#define GUN_H
#include "graphics.h"

#define SHOTGUN_SHOOT_FRAMES 8

extern SDL_Texture *gunTextures[SHOTGUN_SHOOT_FRAMES];

typedef struct {
  int currentFrame;
  double frameTime; // sec per frame
  double timeAccumulator;
  int playing; // 1: playing 0: idle
} GunAnim;

SDL_Texture *loadGunTexture(SDL_Renderer *renderer, const char *filePath);
void loadAllGunTextures(SDL_Renderer *renderer);
void drawGunTexture(Game *game, SDL_Texture *gunTexture, float widthFactor,
                    float heightFactor);
void cleanupGunTextures();

#endif
