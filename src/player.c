#include "player.h"
#include "entities.h"
#include "map.h"
#include "sprites.h"
#include <stdbool.h>
#include <math.h>

static const double PLAYER_FORWARD_ACCEL = 18.0;
static const double PLAYER_FORWARD_DECEL = 22.0;

static const double PLAYER_STRAFE_ACCEL = 16.0;
static const double PLAYER_STRAFE_DECEL = 20.0;
static const double PLAYER_STRAFE_RATIO = 0.8;

static void player_resolve_enemy_collisions(
    Player *player, int worldMap[MAP_HEIGHT][MAP_WIDTH], Sprite *sprites,
    int spriteCount)
{
  const double playerRadius = 0.23;
  const double enemyBaseRadius = 0.25;

  for (int iter = 0; iter < 2; ++iter)
  {
    bool adjusted = false;
    for (int i = 0; i < spriteCount; ++i)
    {
      Sprite *sprite = &sprites[i];
      if (!sprite->active || sprite->kind != SPRITE_ENEMY || sprite->health <= 0)
        continue;

      double dx = player->posX - sprite->x;
      double dy = player->posY - sprite->y;
      double enemyRadius = enemyBaseRadius * sprite->scale;
      double minDistance = playerRadius + enemyRadius;
      double minDistanceSq = minDistance * minDistance;
      double distSq = dx * dx + dy * dy;
      if (distSq >= minDistanceSq)
        continue;

      double nx = dx;
      double ny = dy;
      double dist = sqrt(distSq);
      if (dist < 1e-6)
      {
        nx = player->dirX;
        ny = player->dirY;
        double len = sqrt(nx * nx + ny * ny);
        if (len < 1e-6)
        {
          nx = 1.0;
          ny = 0.0;
          len = 1.0;
        }
        nx /= len;
        ny /= len;
        dist = 0.0;
      }
      else
      {
        nx /= dist;
        ny /= dist;
      }

      double overlap = minDistance - dist + 1e-4;
      double proposedX = player->posX + nx * overlap;
      double proposedY = player->posY + ny * overlap;

      if (worldMap[(int)proposedX][(int)player->posY] == 0)
        player->posX = proposedX;
      if (worldMap[(int)player->posX][(int)proposedY] == 0)
        player->posY = proposedY;

      adjusted = true;
    }
    if (!adjusted)
      break;
  }
}

Player createPlayer() {
  Player p = {POS_X, POS_Y, DIR_X, DIR_Y, PLANE_X, PLANE_Y,
              0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
              HEALTH, MOVE_SPEED, ROT_SPEED, SENS_X, SENS_Y, PITCH,
              1, 0, TOTAL_GUNS, 0};
  return p;
}

void player_applySpawn(Player *player)
{
  if (!player)
    return;

  double spawnX = player->posX;
  double spawnY = player->posY;
  double spawnDirDegrees = 0.0;
  entities_getPlayerSpawn(&spawnX, &spawnY, &spawnDirDegrees);

  player->posX = spawnX;
  player->posY = spawnY;

  const double degToRad = 3.14159265358979323846 / 180.0;
  double dirRad = spawnDirDegrees * degToRad;
  player->dirX = cos(dirRad);
  player->dirY = -sin(dirRad);

  double planeScale = hypot(PLANE_X, PLANE_Y);
  if (planeScale <= 0.0)
    planeScale = 0.88;
  player->planeX = player->dirY * planeScale;
  player->planeY = -player->dirX * planeScale;
}

void player_respawn(Player *player)
{
  if (!player)
    return;

  player->health = HEALTH;
  player->velocityForward = 0.0;
  player->velocityStrafe = 0.0;
  player->velX = 0.0;
  player->velY = 0.0;
  player->damageFlashTimer = 0.0;
  player->bobTime = 0.0;
  player->pitch = PITCH;
  player->mouseHeld = 0;
  player->shooting = 0;
  player->selectedGun = SHOTGUN;
  player->gunsTotal = TOTAL_GUNS;

  player_applySpawn(player);
}

void player_move(Player *player, double deltaTime,
                 int worldMap[MAP_HEIGHT][MAP_WIDTH], Sprite *sprites,
                 int spriteCount, int direction) {
  if (!player)
    return;

  if (player->health <= 0)
  {
    player->velocityForward = 0.0;
    player->velX = 0.0;
    player->velY = 0.0;
    return;
  }

  if (deltaTime <= 0.0)
    return;

  double desired = player->moveSpeed * (double)direction;
  double current = player->velocityForward;
  double maxDelta;
  if (direction == 0)
  {
    maxDelta = PLAYER_FORWARD_DECEL * deltaTime;
  }
  else if ((current > desired && direction > 0) ||
           (current < desired && direction < 0) ||
           (current * desired < 0.0))
  {
    maxDelta = PLAYER_FORWARD_DECEL * deltaTime;
  }
  else
  {
    maxDelta = PLAYER_FORWARD_ACCEL * deltaTime;
  }

  double diff = desired - current;
  if (diff > maxDelta)
    diff = maxDelta;
  else if (diff < -maxDelta)
    diff = -maxDelta;
  player->velocityForward += diff;

  double maxSpeed = player->moveSpeed;
  if (player->velocityForward > maxSpeed)
    player->velocityForward = maxSpeed;
  else if (player->velocityForward < -maxSpeed)
    player->velocityForward = -maxSpeed;

  if (fabs(player->velocityForward) < 1e-4)
    player->velocityForward = 0.0;

  double moveStep = player->velocityForward * deltaTime;

  double oldX = player->posX;
  double oldY = player->posY;

  if (fabs(moveStep) >= 1e-7)
  {
    double newX = player->posX + player->dirX * moveStep;
    double newY = player->posY + player->dirY * moveStep;

    if (!worldMap[(int)newX][(int)player->posY])
      player->posX = newX;
    if (!worldMap[(int)player->posX][(int)newY])
      player->posY = newY;
  }

  double dx = player->posX - oldX;
  double dy = player->posY - oldY;
  if (fabs(moveStep) >= 1e-7 && fabs(dx) < 1e-6 && fabs(dy) < 1e-6)
    player->velocityForward = 0.0;
  if (deltaTime > 0.0)
  {
    player->velX += dx / deltaTime;
    player->velY += dy / deltaTime;
  }

  player_resolve_enemy_collisions(player, worldMap, sprites, spriteCount);
}

void player_strafe(Player *player, double deltaTime,
                   int worldMap[MAP_HEIGHT][MAP_WIDTH], Sprite *sprites,
                   int spriteCount, int direction) {
  if (!player)
    return;

  if (player->health <= 0)
  {
    player->velocityStrafe = 0.0;
    player->velX = 0.0;
    player->velY = 0.0;
    return;
  }

  if (deltaTime <= 0.0)
    return;

  double baseSpeed = player->moveSpeed * PLAYER_STRAFE_RATIO;
  double desired = baseSpeed * (double)direction;
  double current = player->velocityStrafe;
  double maxDelta;
  if (direction == 0)
  {
    maxDelta = PLAYER_STRAFE_DECEL * deltaTime;
  }
  else if ((current > desired && direction > 0) ||
           (current < desired && direction < 0) ||
           (current * desired < 0.0))
  {
    maxDelta = PLAYER_STRAFE_DECEL * deltaTime;
  }
  else
  {
    maxDelta = PLAYER_STRAFE_ACCEL * deltaTime;
  }

  double diff = desired - current;
  if (diff > maxDelta)
    diff = maxDelta;
  else if (diff < -maxDelta)
    diff = -maxDelta;
  player->velocityStrafe += diff;

  double maxSpeed = baseSpeed;
  if (player->velocityStrafe > maxSpeed)
    player->velocityStrafe = maxSpeed;
  else if (player->velocityStrafe < -maxSpeed)
    player->velocityStrafe = -maxSpeed;

  if (fabs(player->velocityStrafe) < 1e-4)
    player->velocityStrafe = 0.0;

  double moveStep = player->velocityStrafe * deltaTime;

  double oldX = player->posX;
  double oldY = player->posY;

  if (fabs(moveStep) >= 1e-7)
  {
    double newX = player->posX + player->planeX * moveStep;
    double newY = player->posY + player->planeY * moveStep;

    if (!worldMap[(int)newX][(int)player->posY])
      player->posX = newX;
    if (!worldMap[(int)player->posX][(int)newY])
      player->posY = newY;
  }

  double dx = player->posX - oldX;
  double dy = player->posY - oldY;
  if (fabs(moveStep) >= 1e-7 && fabs(dx) < 1e-6 && fabs(dy) < 1e-6)
    player->velocityStrafe = 0.0;
  if (deltaTime > 0.0)
  {
    player->velX += dx / deltaTime;
    player->velY += dy / deltaTime;
  }

  player_resolve_enemy_collisions(player, worldMap, sprites, spriteCount);
}

void player_rotate(Player *player, double rotationAmount) {
  double rotStep = rotationAmount;
  double cosRot = cos(rotStep);
  double sinRot = sin(rotStep);

  double oldDirX = player->dirX;
  player->dirX = player->dirX * cosRot - player->dirY * sinRot;
  player->dirY = oldDirX * sinRot + player->dirY * cosRot;

  double oldPlaneX = player->planeX;
  player->planeX = player->planeX * cosRot - player->planeY * sinRot;
  player->planeY = oldPlaneX * sinRot + player->planeY * cosRot;
}

// get rotation Amount for player_rotate
double key_rotationAmount(double rotSpeed, double deltaTime, int direction) {
  double rotationAmount = rotSpeed * deltaTime * direction;
  return rotationAmount;
}

double mouse_rotationAmount(double sensX, Sint16 xrel) {
  double rotationAmount = xrel * sensX;
  return rotationAmount;
}
