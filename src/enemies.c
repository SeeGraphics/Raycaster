#include "enemies.h"
#include "blood.h"
#include "engine.h"
#include "map.h"
#include "sprites.h"
#include <float.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

static const f64 SPRITE_BASE_HIT_RADIUS = 0.30;
static const double ENEMY_MOVE_SPEED = 1.6;
static const double ENEMY_ATTACK_RANGE = 1.25;
static const double ENEMY_ATTACK_COOLDOWN = 1.0;
static const int ENEMY_ATTACK_DAMAGE = 10;

typedef enum
{
  ENEMY_TYPE_NONE = 0,
  ENEMY_TYPE_DEMON,
} EnemyType;

typedef enum
{
  ENEMY_STATE_CHASE = 0,
  ENEMY_STATE_ATTACK,
  ENEMY_STATE_HIT,
  ENEMY_STATE_DYING,
  ENEMY_STATE_DEAD,
} EnemyState;

typedef struct
{
  Sprite *sprite;
  EnemyType type;
  EnemyState state;
  Animation walk;
  Animation attack;
  Animation hit;
  Animation death;
  Animation *current;
  double attackCooldown;
  bool attackDamageApplied;
  bool inUse;
} EnemyController;

static EnemyController g_enemyControllers[NUM_SPRITES];

#define ASTAR_MAX_NODES (MAP_WIDTH * MAP_HEIGHT)

static Animation animation_clone_template(const Animation *source, int playing,
                                          int loopingOverride)
{
  Animation clone;
  memset(&clone, 0, sizeof(Animation));
  if (!source)
    return clone;
  clone = *source;
  clone.currentFrame = 0;
  clone.timeAccumulator = 0.0;
  clone.playing = playing;
  clone.looping = (loopingOverride >= 0) ? loopingOverride : source->looping;
  return clone;
}

static void animation_update_instance(Animation *animation, double deltaTime)
{
  if (!animation || !animation->frames || animation->frameCount <= 0)
    return;

  if (!animation->playing)
    return;

  animation->timeAccumulator += deltaTime;
  if (animation->timeAccumulator < animation->frameTime)
    return;

  animation->timeAccumulator -= animation->frameTime;
  animation->currentFrame++;

  if (animation->currentFrame < animation->frameCount)
    return;

  if (animation->looping)
  {
    animation->currentFrame = 0;
    return;
  }

  animation->currentFrame = 0;
  animation->playing = 0;
}

static EnemyController *enemy_controller_from_sprite(Sprite *sprite)
{
  if (!sprite)
    return NULL;
  if (sprite->auxTextureId < 0 || sprite->auxTextureId >= NUM_SPRITES)
    return NULL;
  EnemyController *controller = &g_enemyControllers[sprite->auxTextureId];
  if (!controller->inUse || controller->sprite != sprite)
    return NULL;
  return controller;
}

static void enemy_set_animation(Sprite *sprite, EnemyController *controller,
                                Animation *anim, bool restart)
{
  if (!sprite || !controller || !anim)
    return;
  if (restart)
  {
    anim->currentFrame = 0;
    anim->timeAccumulator = 0.0;
  }
  anim->playing = 1;
  controller->current = anim;
  sprite->appearance = spriteAppearanceFromAnimation(anim);
}

static void enemy_change_state(Sprite *sprite, EnemyController *controller,
                               EnemyState state, bool restart)
{
  if (!controller || controller->state == ENEMY_STATE_DEAD)
    return;

  controller->state = state;
  controller->attackDamageApplied = false;

  switch (state)
  {
  case ENEMY_STATE_CHASE:
    controller->attack.playing = 0;
    controller->hit.playing = 0;
    controller->death.playing = 0;
    controller->walk.looping = 1;
    enemy_set_animation(sprite, controller, &controller->walk, restart);
    break;
  case ENEMY_STATE_ATTACK:
    controller->walk.playing = 0;
    controller->hit.playing = 0;
    controller->death.playing = 0;
    controller->attack.looping = 0;
    enemy_set_animation(sprite, controller, &controller->attack, true);
    break;
  case ENEMY_STATE_HIT:
    controller->walk.playing = 0;
    controller->attack.playing = 0;
    controller->death.playing = 0;
    controller->hit.looping = 0;
    enemy_set_animation(sprite, controller, &controller->hit, true);
    break;
  case ENEMY_STATE_DYING:
    controller->walk.playing = 0;
    controller->attack.playing = 0;
    controller->hit.playing = 0;
    controller->death.looping = 0;
    enemy_set_animation(sprite, controller, &controller->death, true);
    break;
  case ENEMY_STATE_DEAD:
    controller->walk.playing = 0;
    controller->attack.playing = 0;
    controller->hit.playing = 0;
    controller->death.playing = 0;
    controller->current = NULL;
    break;
  }
}

static void enemy_begin_attack(Sprite *sprite, EnemyController *controller)
{
  if (!controller || controller->state == ENEMY_STATE_DYING)
    return;
  controller->attackDamageApplied = false;
  enemy_change_state(sprite, controller, ENEMY_STATE_ATTACK, true);
}

static void enemy_begin_hit(Sprite *sprite, EnemyController *controller)
{
  if (!controller || controller->state == ENEMY_STATE_DYING)
    return;
  enemy_change_state(sprite, controller, ENEMY_STATE_HIT, true);
}

static void enemy_begin_death(Engine *engine, Sprite *sprite,
                              EnemyController *controller)
{
  if (!controller || controller->state == ENEMY_STATE_DYING ||
      controller->state == ENEMY_STATE_DEAD)
    return;
  enemy_change_state(sprite, controller, ENEMY_STATE_DYING, true);
  if (engine && controller->type == ENEMY_TYPE_DEMON)
    playDemonDeath(&engine->sound);
}

static void enemy_finalize_death(Sprite *sprite, EnemyController *controller)
{
  if (!controller)
    return;
  enemy_change_state(sprite, controller, ENEMY_STATE_DEAD, false);
  if (sprite)
    sprite->active = 0;
  if (controller->sprite)
    controller->sprite->auxTextureId = -1;
  controller->sprite = NULL;
  controller->inUse = false;
}

void enemies_clearControllers(void)
{
  for (int i = 0; i < NUM_SPRITES; ++i)
  {
    EnemyController *controller = &g_enemyControllers[i];
    if (controller->sprite)
      controller->sprite->auxTextureId = -1;
    memset(controller, 0, sizeof(EnemyController));
  }
}

void enemies_registerSprite(Sprite *sprite, const char *animationName)
{
  if (!sprite)
    return;

  EnemyType type = ENEMY_TYPE_NONE;
  if (animationName && strcmp(animationName, "DEMON_WALK") == 0)
    type = ENEMY_TYPE_DEMON;

  if (type == ENEMY_TYPE_NONE)
  {
    fprintf(stderr, "\033[33m[WARN] Unknown enemy animation '%s'\033[0m\n",
            animationName ? animationName : "(null)");
    return;
  }

  if (type == ENEMY_TYPE_DEMON && !animations.demon_walk.frames)
    loadAllAnimations();

  for (int i = 0; i < NUM_SPRITES; ++i)
  {
    EnemyController *controller = &g_enemyControllers[i];
    if (controller->inUse)
      continue;

    memset(controller, 0, sizeof(EnemyController));
    controller->inUse = true;
    controller->sprite = sprite;
    controller->type = type;
    sprite->auxTextureId = i;

    if (type == ENEMY_TYPE_DEMON)
    {
      controller->walk =
          animation_clone_template(&animations.demon_walk, 1, 1);
      controller->attack =
          animation_clone_template(&animations.demon_attack, 0, 0);
      controller->hit = animation_clone_template(&animations.demon_hit, 0, 0);
      controller->death =
          animation_clone_template(&animations.demon_death, 0, 0);
    }

    controller->current = &controller->walk;
    controller->state = ENEMY_STATE_CHASE;
    controller->attackCooldown = 0.0;
    controller->attackDamageApplied = false;
    enemy_set_animation(sprite, controller, controller->current, true);
    return;
  }

  fprintf(stderr, "\033[31m[ERROR] No free enemy controller slots available\033[0m\n");
}

typedef struct
{
  int x;
  int y;
} GridCoord;

static inline int coord_index(int x, int y)
{
  return y * MAP_WIDTH + x;
}

static bool tile_walkable(int x, int y)
{
  if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT)
    return false;
  return worldMap[x][y] <= 0;
}

static double heuristic_cost(int x1, int y1, int x2, int y2)
{
  return fabs((double)x1 - (double)x2) + fabs((double)y1 - (double)y2);
}

static int astar_find_path(int startX, int startY, int goalX, int goalY,
                           GridCoord *outPath, int maxPath)
{
  if (!outPath || maxPath <= 0)
    return 0;

  if (startX == goalX && startY == goalY)
  {
    outPath[0].x = startX;
    outPath[0].y = startY;
    return 1;
  }

  double gScore[ASTAR_MAX_NODES];
  double fScore[ASTAR_MAX_NODES];
  int cameFrom[ASTAR_MAX_NODES];
  bool inOpen[ASTAR_MAX_NODES];
  bool inClosed[ASTAR_MAX_NODES];

  for (int i = 0; i < ASTAR_MAX_NODES; ++i)
  {
    gScore[i] = DBL_MAX;
    fScore[i] = DBL_MAX;
    cameFrom[i] = -1;
    inOpen[i] = false;
    inClosed[i] = false;
  }

  int openList[ASTAR_MAX_NODES];
  int openCount = 0;

  if (!tile_walkable(startX, startY))
    return 0;

  int startIndex = coord_index(startX, startY);
  int goalIndex = coord_index(goalX, goalY);

  gScore[startIndex] = 0.0;
  fScore[startIndex] = heuristic_cost(startX, startY, goalX, goalY);
  openList[openCount++] = startIndex;
  inOpen[startIndex] = true;

  static const int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

  while (openCount > 0)
  {
    int bestIndex = -1;
    double bestF = DBL_MAX;
    for (int i = 0; i < openCount; ++i)
    {
      int node = openList[i];
      if (fScore[node] < bestF)
      {
        bestF = fScore[node];
        bestIndex = i;
      }
    }

    if (bestIndex < 0)
      break;

    int current = openList[bestIndex];
    int currentX = current % MAP_WIDTH;
    int currentY = current / MAP_WIDTH;

    openCount--;
    for (int i = bestIndex; i < openCount; ++i)
      openList[i] = openList[i + 1];
    inOpen[current] = false;
    inClosed[current] = true;

    if (current == goalIndex)
    {
      int length = 0;
      int node = goalIndex;
      while (node >= 0)
      {
        if (length >= maxPath)
          return 0;
        outPath[length].x = node % MAP_WIDTH;
        outPath[length].y = node / MAP_WIDTH;
        if (node == startIndex)
          break;
        node = cameFrom[node];
        length++;
      }

      if (node != startIndex)
        return 0;

      length++;
      for (int i = 0; i < length / 2; ++i)
      {
        GridCoord tmp = outPath[i];
        outPath[i] = outPath[length - 1 - i];
        outPath[length - 1 - i] = tmp;
      }
      return length;
    }

    for (int d = 0; d < 4; ++d)
    {
      int nx = currentX + directions[d][0];
      int ny = currentY + directions[d][1];
      if (nx < 0 || ny < 0 || nx >= MAP_WIDTH || ny >= MAP_HEIGHT)
        continue;
      if (!tile_walkable(nx, ny) && coord_index(nx, ny) != goalIndex)
        continue;

      int neighbor = coord_index(nx, ny);
      if (inClosed[neighbor])
        continue;

      double tentativeG = gScore[current] + 1.0;
      if (!inOpen[neighbor])
      {
        openList[openCount++] = neighbor;
        inOpen[neighbor] = true;
      }
      else if (tentativeG >= gScore[neighbor])
      {
        continue;
      }

      cameFrom[neighbor] = current;
      gScore[neighbor] = tentativeG;
      fScore[neighbor] = tentativeG + heuristic_cost(nx, ny, goalX, goalY);
    }
  }

  return 0;
}

static void enemy_move_towards(Sprite *enemy, double targetX, double targetY,
                               double deltaTime)
{
  double dx = targetX - enemy->x;
  double dy = targetY - enemy->y;
  double dist = sqrt(dx * dx + dy * dy);
  if (dist < 1e-5)
  {
    enemy->x = targetX;
    enemy->y = targetY;
    return;
  }

  double maxStep = ENEMY_MOVE_SPEED * deltaTime;
  if (dist <= maxStep)
  {
    enemy->x = targetX;
    enemy->y = targetY;
  }
  else
  {
    double scale = maxStep / dist;
    enemy->x += dx * scale;
    enemy->y += dy * scale;
  }
}

static void enemy_update_path_follow(Engine *engine, Sprite *enemy,
                                     double deltaTime)
{
  if (!engine || !enemy)
    return;

  int startX = (int)floor(enemy->x);
  int startY = (int)floor(enemy->y);
  int goalX = (int)floor(engine->player.posX);
  int goalY = (int)floor(engine->player.posY);

  if (startX == goalX && startY == goalY)
    return;

  GridCoord path[ASTAR_MAX_NODES];
  int pathLen = astar_find_path(startX, startY, goalX, goalY, path,
                                ASTAR_MAX_NODES);
  if (pathLen <= 1)
    return;

  int nextX = path[1].x;
  int nextY = path[1].y;
  enemy->targetX = nextX;
  enemy->targetY = nextY;

  double targetWorldX = (double)nextX + 0.5;
  double targetWorldY = (double)nextY + 0.5;
  enemy_move_towards(enemy, targetWorldX, targetWorldY, deltaTime);
}

static void enemy_update_controller(Engine *engine, Sprite *sprite,
                                    EnemyController *controller,
                                    double deltaTime)
{
  if (!engine || !sprite || !controller)
    return;

  if (controller->state != ENEMY_STATE_DYING && sprite->health <= 0)
    enemy_begin_death(engine, sprite, controller);

  if (controller->state == ENEMY_STATE_DEAD)
    return;

  if (controller->attackCooldown > 0.0)
  {
    controller->attackCooldown -= deltaTime;
    if (controller->attackCooldown < 0.0)
      controller->attackCooldown = 0.0;
  }

  double dx = engine->player.posX - sprite->x;
  double dy = engine->player.posY - sprite->y;
  double distanceToPlayer = sqrt(dx * dx + dy * dy);

  switch (controller->state)
  {
  case ENEMY_STATE_CHASE:
    animation_update_instance(&controller->walk, deltaTime);
    if (sprite->health > 0)
      enemy_update_path_follow(engine, sprite, deltaTime);
    if (sprite->health > 0 && distanceToPlayer <= ENEMY_ATTACK_RANGE &&
        controller->attackCooldown <= 0.0)
    {
      enemy_begin_attack(sprite, controller);
      return;
    }
    break;
  case ENEMY_STATE_ATTACK:
    animation_update_instance(&controller->attack, deltaTime);
    if (!controller->attackDamageApplied &&
        controller->attack.frameCount > 0 &&
        controller->attack.currentFrame >= controller->attack.frameCount / 2)
    {
      if (distanceToPlayer <= ENEMY_ATTACK_RANGE)
      {
        int prevHealth = engine->player.health;
        engine->player.health -= ENEMY_ATTACK_DAMAGE;
        if (engine->player.health < 0)
          engine->player.health = 0;
        engine->player.damageFlashTimer = PLAYER_DAMAGE_FLASH_DURATION;
        if (engine->player.health <= 0)
        {
          if (prevHealth > 0)
            playPlayerDeath(&engine->sound);
          engine->player.mouseHeld = 0;
          engine->player.shooting = 0;
          engine->player.velocityForward = 0.0;
          engine->player.velocityStrafe = 0.0;
          engine->player.velX = 0.0;
          engine->player.velY = 0.0;
        }
        else if (engine->player.health < prevHealth)
        {
          playPlayerHit(&engine->sound);
        }
      }
      controller->attackDamageApplied = true;
    }
    if (!controller->attack.playing)
    {
      controller->attackCooldown = ENEMY_ATTACK_COOLDOWN;
      enemy_change_state(sprite, controller, ENEMY_STATE_CHASE, true);
    }
    break;
  case ENEMY_STATE_HIT:
    animation_update_instance(&controller->hit, deltaTime);
    if (!controller->hit.playing)
    {
      if (sprite->health <= 0)
        enemy_begin_death(engine, sprite, controller);
      else
        enemy_change_state(sprite, controller, ENEMY_STATE_CHASE, true);
    }
    break;
  case ENEMY_STATE_DYING:
    animation_update_instance(&controller->death, deltaTime);
    if (!controller->death.playing)
      enemy_finalize_death(sprite, controller);
    break;
  case ENEMY_STATE_DEAD:
    break;
  }

  if (controller->current)
    sprite->appearance.anim.animation = controller->current;
}

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

void enemies_update(Engine *engine, double deltaTime)
{
  if (!engine || !engine->sprites || deltaTime <= 0.0)
    return;

  for (int i = 0; i < NUM_SPRITES; ++i)
  {
    Sprite *sprite = &engine->sprites[i];
    if (sprite->kind != SPRITE_ENEMY)
      continue;

    EnemyController *controller = enemy_controller_from_sprite(sprite);
    if (!controller)
      continue;

    if (!sprite->active && controller->state != ENEMY_STATE_DYING)
      continue;

    enemy_update_controller(engine, sprite, controller, deltaTime);
  }
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

  EnemyController *controller = enemy_controller_from_sprite(target);
  if (!controller)
  {
    target->health -= damage;
    double hitDirX = target->x - engine->player.posX;
    double hitDirY = target->y - engine->player.posY;
    blood_spawnBurst(target->x, target->y, hitDirX, hitDirY);
    if (target->health <= 0)
    {
      target->health = 0;
      target->active = 0;
    }
    return;
  }

  if (controller->state == ENEMY_STATE_DYING ||
      controller->state == ENEMY_STATE_DEAD)
    return;

  target->health -= damage;
  double hitDirX = target->x - engine->player.posX;
  double hitDirY = target->y - engine->player.posY;
  blood_spawnBurst(target->x, target->y, hitDirX, hitDirY);
  if (target->health <= 0)
  {
    target->health = 0;
    enemy_begin_death(engine, target, controller);
  }
  else
  {
    enemy_begin_hit(target, controller);
    if (controller->attackCooldown < 0.2)
      controller->attackCooldown = 0.2;
  }
}
