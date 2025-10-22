#include "gun.h"
#include "SDL_image.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

SDL_Texture *gunTextures[SHOTGUN_SHOOT_FRAMES] = {NULL};
SDL_Texture *reloadTextures[SHOTGUN_RELOAD_FRAMES] = {NULL};

SDL_Texture *loadGunTexture(SDL_Renderer *renderer, const char *filePath) {
  SDL_Surface *surface = IMG_Load(filePath);
  if (!surface) {
    printf("Failed to load gun image '%s': %s\n", filePath, IMG_GetError());
    return NULL;
  }

  // Set color key for transparency (green background #99E550)
  Uint32 green = SDL_MapRGB(surface->format, 0x99, 0xE5, 0x50);
  SDL_SetColorKey(surface, SDL_TRUE, green);

  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  if (!texture) {
    printf("Failed to create texture for '%s': %s\n", filePath, SDL_GetError());
    return NULL;
  }

  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

  return texture;
}

void loadAllTextures_Shotgun_shoot(SDL_Renderer *renderer) {
  char filePath[256];
  for (int i = 0; i < SHOTGUN_SHOOT_FRAMES; i++) {
    snprintf(filePath, sizeof(filePath),
             "assets/textures/weapons/shotgun/shoot/%d.png", i);
    gunTextures[i] = loadGunTexture(renderer, filePath);
    if (!gunTextures[i]) {
      fprintf(stderr, "Failed to load gun frame %d!\n", i);
    }
  }
}

void loadAllTextures_Shotgun_reload(SDL_Renderer *renderer) {
  char filePath[256];
  for (int i = 0; i < SHOTGUN_RELOAD_FRAMES; i++) {
    snprintf(filePath, sizeof(filePath),
             "assets/textures/weapons/shotgun/reload/%d.png", i);
    reloadTextures[i] = loadGunTexture(renderer, filePath);
    if (!reloadTextures[i]) {
      fprintf(stderr, "Failed to load gun reload frame %d!\n", i);
    }
  }
}

void drawGunTexture(Game *game, SDL_Texture *gunTexture, float widthFactor,
                    float heightFactor) {
  if (!gunTexture)
    return;

  int winW = game->window_width;
  int winH = game->window_height;

  // scale relative to screen size
  int gunW = (int)(winW * widthFactor);
  int gunH = (int)(winH * heightFactor);

  // centered horizontally, bottom-aligned
  SDL_Rect dstRect = {(winW - gunW) / 2, winH - gunH, gunW, gunH};
  SDL_RenderCopy(game->renderer, gunTexture, NULL, &dstRect);
}

void cleanupGunTextures() {
  for (int i = 0; i < SHOTGUN_SHOOT_FRAMES; i++) {
    if (gunTextures[i]) {
      SDL_DestroyTexture(gunTextures[i]);
      gunTextures[i] = NULL;
    }
  }
}
