#ifndef PLAYER_H
#define PLAYER_H

#include "SDL.h"
#include "map.h"
#include <stdbool.h>

#define CLAMP 120
#define POS_X 20.3
#define POS_Y 20.5
#define DIR_X -1.0
#define DIR_Y 0.0
#define PLANE_X 0.0
#define PLANE_Y 0.88
#define MOVE_SPEED 5.0
#define ROT_SPEED 3.0
#define SENS_X 0.002
#define SENS_Y 90.0
#define PITCH 0.0

typedef struct {
  double posX, posY;
  double dirX, dirY;
  double planeX, planeY;
  double moveSpeed;
  double rotSpeed;
  double sensX;
  double sensY;
  double pitch;
} Player;

// movement
void player_move(Player *player, double deltaTime,
                 int worldMap[MAP_HEIGHT][MAP_WIDTH], int direction);
void player_strafe(Player *player, double deltaTime,
                   int worldMap[MAP_HEIGHT][MAP_WIDTH], int direction);

// rotation
void player_rotate(Player *player, double rotationAmount);
double mouse_rotationAmount(double sensX, Sint16 xrel);
double key_rotationAmount(double rotSpeed, double deltaTime, int direction);

#endif
