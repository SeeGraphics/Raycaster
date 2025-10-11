#ifndef PLAYER_H
#define PLAYER_H

#include "SDL.h"
#include "map.h"
#include <stdbool.h>

typedef struct {
  double posX, posY;
  double dirX, dirY;
  double planeX, planeY;
  double moveSpeed;
  double rotSpeed;    // arrow keys sens
  double sensitivity; // mouse sens
} Player;

// movement
void player_move(Player *player, double deltaTime,
                 int worldMap[MAP_HEIGHT][MAP_WIDTH], int direction);
void player_strafe(Player *player, double deltaTime,
                   int worldMap[MAP_HEIGHT][MAP_WIDTH], int direction);

// rotation
void player_rotate(Player *player, double rotationAmount);
double mouse_rotationAmount(double sensitivity, Sint16 xrel);
double key_rotationAmount(double rotSpeed, double deltaTime, int direction);

#endif
