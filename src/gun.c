#include "gun.h"
#include <SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

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
