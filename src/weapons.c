#include "weapons.h"

WeaponProperties weaponProperties[TOTAL_GUNS] = {
    {0, 0.0, 0.0, 100, 45},                     // 0 semi: SHOTGUN
    {0, 0.0, 0.0, 50, 120},                     // 1 semi: ROCKET
    {0, 0.0, 0.0, 150, 15},                     // 2 semi: PISTOL
    {0, 0.0, 0.0, 80, 30},                      // 3 semi: SINGLE
    {1, FRAMETIME_MINIGUN_SHOOT, 0.0, 500, 6},  // 4 automatic: MINIGUN
};
