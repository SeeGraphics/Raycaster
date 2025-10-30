#include "enemies.h"
#include "engine.h"
#include "map.h"
#include "sprites.h"
#include <float.h>
#include <math.h>

static const f64 SPRITE_BASE_HIT_RADIUS = 0.45;

static f64 hitscan_distance_to_wall(const Engine *engine, f64 dirX, f64 dirY)
{
  if (!engine)
    return -1.0;

  f64 posX = engine->player.posX;
  f64 posY = engine->player.posY;

  i32 mapX = (i32)posX;
  i32 mapY = (i32)posY;

  if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT)
    return -1.0;

  f64 deltaDistX = (dirX == 0.0) ? DBL_MAX : fabs(1.0 / dirX);
  f64 deltaDistY = (dirY == 0.0) ? DBL_MAX : fabs(1.0 / dirY);

  i32 stepX = (dirX < 0.0) ? -1 : 1;
  i32 stepY = (dirY < 0.0) ? -1 : 1;

  f64 sideDistX = (dirX < 0.0) ? (posX - mapX) * deltaDistX
                               : ((mapX + 1.0) - posX) * deltaDistX;
  f64 sideDistY = (dirY < 0.0) ? (posY - mapY) * deltaDistY
                               : ((mapY + 1.0) - posY) * deltaDistY;

  i32 side = 0;
  for (i32 step = 0; step < 256; ++step)
  {
    if (sideDistX < sideDistY)
    {
      sideDistX += deltaDistX;
      mapX += stepX;
      side = 0;
    }
    else
    {
      sideDistY += deltaDistY;
      mapY += stepY;
      side = 1;
    }

    if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT)
      return -1.0;

    if (worldMap[mapX][mapY] > 0)
      return (side == 0) ? (sideDistX - deltaDistX)
                         : (sideDistY - deltaDistY);
  }

  return -1.0;
}

static Sprite *find_hitscan_enemy(Engine *engine, f64 dirX, f64 dirY,
                                  f64 wallDistance, f64 *outDistance)
{
  if (!engine)
    return NULL;

  f64 closest = (wallDistance > 0.0) ? wallDistance : DBL_MAX;
  Sprite *target = NULL;

  for (i32 i = 0; i < NUM_SPRITES; ++i)
  {
    Sprite *sprite = &engine->sprites[i];
    if (!sprite->active || sprite->kind != SPRITE_ENEMY || sprite->health <= 0)
      continue;

    f64 dx = sprite->x - engine->player.posX;
    f64 dy = sprite->y - engine->player.posY;

    f64 forward = dx * dirX + dy * dirY;
    if (forward <= 0.0)
      continue;

    f64 lateral = fabs(dx * dirY - dy * dirX);
    f64 radius = SPRITE_BASE_HIT_RADIUS * (f64)sprite->scale;

    if (lateral > radius)
      continue;

    if (wallDistance > 0.0 && forward >= wallDistance)
      continue;

    if (forward < closest)
    {
      closest = forward;
      target = sprite;
    }
  }

  if (target && outDistance)
    *outDistance = closest;

  return target;
}

void enemies_applyHitscanDamage(Engine *engine, i32 damage)
{
  if (!engine || damage <= 0)
    return;

  f64 dirX = engine->player.dirX;
  f64 dirY = engine->player.dirY;

  f64 wallDistance = hitscan_distance_to_wall(engine, dirX, dirY);

  f64 enemyDistance = 0.0;
  Sprite *target =
      find_hitscan_enemy(engine, dirX, dirY, wallDistance, &enemyDistance);
  if (!target)
    return;

  target->health -= damage;
  if (target->health <= 0)
  {
    target->health = 0;
    target->active = 0;

    if (target->appearance.type == SPRITE_VISUAL_ANIMATION &&
        target->appearance.anim.animation)
    {
      target->appearance.anim.animation->playing = 0;
    }
  }
}
