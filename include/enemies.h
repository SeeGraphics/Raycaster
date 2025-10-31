#ifndef ENEMIES_H
#define ENEMIES_H

#include "types.h"

struct Engine;

void enemies_applyHitscanDamage(struct Engine *engine, i32 damage);
void enemies_update(struct Engine *engine, double deltaTime);

#endif
