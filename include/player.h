#ifndef PLAYER_H
#define PLAYER_H

#include "SDL.h"
#include "map.h"
#include <stdbool.h>

#define CLAMP 160
#define POS_X 22.0
#define POS_Y 11.5
#define DIR_X -1.0
#define DIR_Y 0.0
#define PLANE_X 0.0
#define PLANE_Y 0.88
#define HEALTH 100
#define MOVE_SPEED 5.0
#define ROT_SPEED 3.0
#define SENS_X 0.002
#define SENS_Y 90.0
#define PITCH 0.0

typedef enum {
  SHOTGUN,
  ROCKET,
  PISTOL,
  SINGLE,
  MINIGUN,
  TOTAL_GUNS,
} SELECTED_GUN;

typedef struct {
  double posX, posY;
  double dirX, dirY;
  double planeX, planeY;
  int health;
  double moveSpeed;
  double rotSpeed;
  double sensX;
  double sensY;
  double pitch;
  SELECTED_GUN selectedGun;
  int shooting; // to not allow gun swap when still in shooting animation
  int gunsTotal;
  int mouseHeld;
} Player;

// init
Player createPlayer();

// movement
void player_move(Player *player, double deltaTime,
                 int worldMap[MAP_HEIGHT][MAP_WIDTH], int direction);
void player_strafe(Player *player, double deltaTime,
                   int worldMap[MAP_HEIGHT][MAP_WIDTH], int direction);

void player_rotate(Player *player, double rotationAmount);
double mouse_rotationAmount(double sensX, Sint16 xrel);
double key_rotationAmount(double rotSpeed, double deltaTime, int direction);

#endif
