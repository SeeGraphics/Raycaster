#ifndef SPRITES_H
#define SPRITES_H

#define NUM_SPRITES 19

typedef struct {
  double x;
  double y;
  int texture;
} Sprite;

// foward declaration
struct Engine;

// init
Sprite *createSprite();

// calculations
void perform_spritecasting(struct Engine *engine);
void sortSprites(int *order, double *dist, int amount);

#endif
