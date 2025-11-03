#ifndef ENEMIES_H
#define ENEMIES_H

#include "types.h"

struct Engine;
struct Sprite;

void enemies_applyHitscanDamage(struct Engine *engine, i32 damage);
void enemies_update(struct Engine *engine, double deltaTime);
void enemies_registerSprite(struct Sprite *sprite, const char *animationName);
void enemies_clearControllers(void);

#endif
