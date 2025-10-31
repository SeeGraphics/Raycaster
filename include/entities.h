#ifndef ENTITIES_H
#define ENTITIES_H

#include "sprites.h"
#include "types.h"

Sprite *entities_createWorldSprites(void);
Sprite *entities_getSprites(void);
i32 entities_getSpriteCount(void);
void entities_reset(void);
void entities_getPlayerSpawn(double *outX, double *outY, double *outDirDegrees);

#endif
