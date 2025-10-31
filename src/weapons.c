#include "weapons.h"

WeaponProperties weaponProperties[TOTAL_GUNS] = {
    {0, 0.0, 0.0, 100, 55},                     // 0 semi: SHOTGUN
    {0, 0.0, 0.0, 50, 140},                     // 1 semi: ROCKET
    {0, 0.0, 0.0, 150, 20},                     // 2 semi: PISTOL
    {0, 0.0, 0.0, 80, 35},                      // 3 semi: SINGLE
    {1, FRAMETIME_MINIGUN_SHOOT, 0.0, 500, 8},  // 4 automatic: MINIGUN
};
