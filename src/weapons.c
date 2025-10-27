#include "weapons.h"

WeaponProperties weaponProperties[TOTAL_GUNS] = {
    {0, 0.0, 0.0, 100},                     // 0 semi: SHOTGUN
    {0, 0.0, 0.0, 100},                     // 1 semi: ROCKET
    {0, 0.0, 0.0, 100},                     // 2 semi: PISTOL
    {0, 0.0, 0.0, -1},                      // 3 semi: HANDS
    {0, 0.0, 0.0, 100},                     // 4 semi: SINGLE
    {1, FRAMETIME_MINIGUN_SHOOT, 0.0, 100}, // 5 automatic: MINIGUN
};
