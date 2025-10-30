#ifndef WEAPONS_H
#define WEAPONS_H

#include "animation.h"
#include "player.h"
#include "types.h"

typedef struct
{
  i32 automatic;
  f64 fireRate;
  f64 fireAccumulator;
  i32 ammunition;
  i32 damage;
} WeaponProperties;

extern WeaponProperties weaponProperties[TOTAL_GUNS];

#endif
