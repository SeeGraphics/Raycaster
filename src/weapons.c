#include "weapons.h"
#include <string.h>

static const WeaponProperties kDefaultWeaponProperties[TOTAL_GUNS] = {
    {0, 0.0, 0.0, 100, 55},                    // SHOTGUN
    {0, 0.0, 0.0, 50, 140},                    // ROCKET
    {0, 0.0, 0.0, 150, 20},                    // PISTOL
    {0, 0.0, 0.0, 80, 35},                     // SINGLE
    {1, FRAMETIME_MINIGUN_SHOOT, 0.0, 500, 8}, // MINIGUN
};

WeaponProperties weaponProperties[TOTAL_GUNS];

void weapons_resetProperties(void)
{
  memcpy(weaponProperties, kDefaultWeaponProperties, sizeof(weaponProperties));
}
