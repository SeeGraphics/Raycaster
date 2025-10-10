#include "player.h"
#include <math.h>

void player_move_forward(Player *player, double deltaTime,
                         int worldMap[24][24]) {
  double moveStep = player->moveSpeed * deltaTime;
  int mapX = (int)(player->posX + player->dirX * moveStep);
  int mapY = (int)(player->posY);
  if (!worldMap[mapX][mapY])
    player->posX += player->dirX * moveStep;

  mapX = (int)(player->posX);
  mapY = (int)(player->posY + player->dirY * moveStep);
  if (!worldMap[mapX][mapY])
    player->posY += player->dirY * moveStep;
}

void player_move_backward(Player *player, double deltaTime,
                          int worldMap[24][24]) {
  double moveStep = player->moveSpeed * deltaTime;
  int mapX = (int)(player->posX - player->dirX * moveStep);
  int mapY = (int)(player->posY);
  if (!worldMap[mapX][mapY])
    player->posX -= player->dirX * moveStep;

  mapX = (int)(player->posX);
  mapY = (int)(player->posY - player->dirY * moveStep);
  if (!worldMap[mapX][mapY])
    player->posY -= player->dirY * moveStep;
}

void player_rotate_left(Player *player, double deltaTime) {
  double rotStep = player->rotSpeed * deltaTime;
  double oldDirX = player->dirX;
  player->dirX = player->dirX * cos(rotStep) - player->dirY * sin(rotStep);
  player->dirY = oldDirX * sin(rotStep) + player->dirY * cos(rotStep);

  double oldPlaneX = player->planeX;
  player->planeX =
      player->planeX * cos(rotStep) - player->planeY * sin(rotStep);
  player->planeY = oldPlaneX * sin(rotStep) + player->planeY * cos(rotStep);
}

void player_rotate_right(Player *player, double deltaTime) {
  double rotStep = -player->rotSpeed * deltaTime;
  double oldDirX = player->dirX;
  player->dirX = player->dirX * cos(rotStep) - player->dirY * sin(rotStep);
  player->dirY = oldDirX * sin(rotStep) + player->dirY * cos(rotStep);

  double oldPlaneX = player->planeX;
  player->planeX =
      player->planeX * cos(rotStep) - player->planeY * sin(rotStep);
  player->planeY = oldPlaneX * sin(rotStep) + player->planeY * cos(rotStep);
}
