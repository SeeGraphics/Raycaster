#ifndef WEAPONS_H
#define WEAPONS_H

#include "animation.h"
#include "player.h"

typedef struct {
  int automatic;
  double fireRate;
  double fireAccumulator;
  int ammunition;
} WeaponProperties;

extern WeaponProperties weaponProperties[TOTAL_GUNS];

#endif
