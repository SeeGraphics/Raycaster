#ifndef SPRITES_H
#define SPRITES_H

#include "graphics.h"
#include "player.h"
#include "texture.h"

#define NUM_SPRITES 19

typedef struct {
  double x;
  double y;
  int texture;
} Sprite;

void perform_spritecasting(Game *game, Sprite *sprite, TextureManager *tm,
                           Player *player);
void sortSprites(int *order, double *dist, int amount);

#endif
