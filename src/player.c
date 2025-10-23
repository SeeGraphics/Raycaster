#include "player.h"
#include "map.h"
#include <math.h>

Player createPlayer() {
  Player p = {POS_X,      POS_Y,     DIR_X,  DIR_Y,  PLANE_X, PLANE_Y,
              MOVE_SPEED, ROT_SPEED, SENS_X, SENS_Y, PITCH};
  return p;
}

void player_move(Player *player, double deltaTime,
                 int worldMap[MAP_HEIGHT][MAP_WIDTH], int direction) {
  double moveStep = player->moveSpeed * deltaTime * direction;

  double newX = player->posX + player->dirX * moveStep;
  double newY = player->posY + player->dirY * moveStep;

  // collision check
  if (!worldMap[(int)newX][(int)player->posY])
    player->posX = newX;
  if (!worldMap[(int)player->posX][(int)newY])
    player->posY = newY;
}

void player_strafe(Player *player, double deltaTime,
                   int worldMap[MAP_HEIGHT][MAP_WIDTH], int direction) {
  double moveStep = player->moveSpeed * deltaTime * direction;

  double newX = player->posX + player->planeX * moveStep;
  double newY = player->posY + player->planeY * moveStep;

  // collision check
  if (!worldMap[(int)newX][(int)player->posY])
    player->posX = newX;
  if (!worldMap[(int)player->posX][(int)newY])
    player->posY = newY;
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
