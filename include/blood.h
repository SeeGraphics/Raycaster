#ifndef BLOOD_H
#define BLOOD_H

#include "types.h"

struct Engine;

void blood_reset(void);
void blood_spawnBurst(double x, double y, double dirX, double dirY);
void blood_update(double deltaTime);
void blood_render(struct Engine *engine);

#endif
