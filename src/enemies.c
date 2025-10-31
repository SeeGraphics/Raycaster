#include "enemies.h"
#include "engine.h"
#include "map.h"
#include "sprites.h"
#include <float.h>
#include <stdbool.h>
#include <math.h>

static const f64 SPRITE_BASE_HIT_RADIUS = 0.30;
static const double ENEMY_MOVE_SPEED = 1.6;

#define ASTAR_MAX_NODES (MAP_WIDTH * MAP_HEIGHT)

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
    if (!sprite->active || sprite->kind != SPRITE_ENEMY || sprite->health <= 0)
      continue;

    enemy_update_path_follow(engine, sprite, deltaTime);
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

  target->health -= damage;
  if (target->health <= 0)
  {
    target->health = 0;
    target->active = 0;
  }
}
