#ifndef SPRITES_H
#define SPRITES_H

#define NUM_SPRITES 28

// Texture index macros
#define TEX_PILLAR 13
#define TEX_BARREL 14
#define TEX_GREENLIGHT 15

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
