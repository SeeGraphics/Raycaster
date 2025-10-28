#include "weapons.h"

WeaponProperties weaponProperties[TOTAL_GUNS] = {
    {0, 0.0, 0.0, 10},                     // 0 semi: SHOTGUN
    {0, 0.0, 0.0, 10},                     // 1 semi: ROCKET
    {0, 0.0, 0.0, 10},                     // 2 semi: PISTOL
    {0, 0.0, 0.0, 10},                     // 4 semi: SINGLE
    {1, FRAMETIME_MINIGUN_SHOOT, 0.0, 10}, // 5 automatic: MINIGUN
};
