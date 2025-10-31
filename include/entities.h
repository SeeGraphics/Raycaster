#ifndef ENTITIES_H
#define ENTITIES_H

#include "sprites.h"
#include "types.h"

struct Engine;

Sprite *entities_createWorldSprites(void);
Sprite *entities_getSprites(void);
i32 entities_getSpriteCount(void);
void entities_reset(void);
void entities_getPlayerSpawn(double *outX, double *outY, double *outDirDegrees);
void entities_tryInteract(struct Engine *engine);
int entities_getLeverTextureAtFace(int tileX, int tileY, int faceX, int faceY,
                                   int *outActivated);
int entities_getWallTextAt(int tileX, int tileY, int faceX, int faceY,
                           const u32 **outPixels, float *outScale,
                           float *outCoverageX, float *outCoverageY);

#endif
