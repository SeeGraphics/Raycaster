#include "raycast.h"
#include "graphics.h"
#include "map.h"
#include "math.h"
#include "player.h"
#include <math.h>

void perform_raycasting(Game *game, Player *player) {
  for (int x = 0; x < game->window_width; x++) {
    double cameraX = 2 * x / (double)game->window_width - 1;
    double rayDirX = player->dirX + player->planeX * cameraX;
    double rayDirY = player->dirY + player->planeY * cameraX;

    int mapX = (int)player->posX;
    int mapY = (int)player->posY;

    double deltaDistX = (rayDirX == 0) ? 1e30 : fabs(1.0 / rayDirX);
    double deltaDistY = (rayDirY == 0) ? 1e30 : fabs(1.0 / rayDirY);

    double sideDistX = (rayDirX < 0) ? (player->posX - mapX) * deltaDistX
                                     : (mapX + 1.0 - player->posX) * deltaDistX;
    double sideDistY = (rayDirY < 0) ? (player->posY - mapY) * deltaDistY
                                     : (mapY + 1.0 - player->posY) * deltaDistY;

    int stepX = (rayDirX < 0) ? -1 : 1;
    int stepY = (rayDirY < 0) ? -1 : 1;

    int hit = 0;
    int side;

    while (!hit) {
      if (sideDistX < sideDistY) {
        sideDistX += deltaDistX;
        mapX += stepX;
        side = 0;
      } else {
        sideDistY += deltaDistY;
        mapY += stepY;
        side = 1;
      }

      if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT &&
          worldMap[mapX][mapY] > 0) {
        hit = 1;
      }
    }

    double perpWallDist =
        (side == 0) ? sideDistX - deltaDistX : sideDistY - deltaDistY;
    int lineHeight = (int)(game->window_height / perpWallDist);

    int drawStart = -lineHeight / 2 + game->window_height / 2;
    if (drawStart < 0)
      drawStart = 0;

    int drawEnd = lineHeight / 2 + game->window_height / 2;
    if (drawEnd >= game->window_height)
      drawEnd = game->window_height - 1;

    SDL_Color color;
    switch (worldMap[mapX][mapY]) {
    case 1:
      color = RGB_Red;
      break;
    case 2:
      color = RGB_Green;
      break;
    case 3:
      color = RGB_Blue;
      break;
    case 4:
      color = RGB_White;
      break;
    default:
      color = RGB_Yellow;
      break;
    }

    if (side == 1) { // darken y-sides
      color.r /= 2;
      color.g /= 2;
      color.b /= 2;
    }

    verLine(game->renderer, x, drawStart, drawEnd, color);
  }
}
