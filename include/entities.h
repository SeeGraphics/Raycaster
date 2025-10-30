#ifndef ENTITIES_H
#define ENTITIES_H

#include "sprites.h"
#include "types.h"

typedef struct
{
  Sprite *sprites;
  i32 count;
} EntitySet;

Sprite *entities_createWorldSprites(void);
Sprite *entities_getSprites(void);
i32 entities_getSpriteCount(void);
void entities_reset(void);

#endif
