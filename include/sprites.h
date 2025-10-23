#ifndef SPRITES_H
#define SPRITES_H

#include "engine.h"

#define NUM_SPRITES 19

typedef struct {
  double x;
  double y;
  int texture;
} Sprite;

// init
Sprite *createSprite();

// calculations
void perform_spritecasting(Engine *engine);
void sortSprites(int *order, double *dist, int amount);

#endif
