#ifndef PLAYER_H
#define PLAYER_H

#include "map.h"
#include <stdbool.h>

typedef struct {
  double posX;
  double posY;
  double dirX;
  double dirY;
  double planeX;
  double planeY;
  double moveSpeed;
  double rotSpeed;
} Player;

void player_move_forward(Player *player, double deltaTime,
                         int worldMap[MAP_WIDTH][MAP_HEIGHT]);
void player_move_backward(Player *player, double deltaTime,
                          int worldMap[MAP_WIDTH][MAP_HEIGHT]);
void player_rotate_left(Player *player, double deltaTime);
void player_rotate_right(Player *player, double deltaTime);

#endif
