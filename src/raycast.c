#include "raycast.h"
#include "map.h"
#include "texture.h"

void perform_raycasting(Engine *engine) {
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

    int drawStart = -lineHeight / 2 + game->window_height / 2 + player->pitch;
    if (drawStart < 0)
      drawStart = 0;

    int drawEnd = lineHeight / 2 + game->window_height / 2 + player->pitch;
    if (drawEnd >= game->window_height)
      drawEnd = game->window_height - 1;

    // texturing math
    int texNum = worldMap[mapX][mapY] - 1; // -1 so we can use texture 0

    // calculate value of wallX
    double wallX; // where exactly the wall was hit
    if (side == 0)
      wallX = player->posY + perpWallDist * rayDirY;
    else
      wallX = player->posX + perpWallDist * rayDirX;
    wallX -= floor((wallX));

    // x coordinate on the texture
    int texX = (int)(wallX * (double)TEXT_WIDTH);
    if (side == 0 && rayDirX > 0)
      texX = TEXT_WIDTH - texX - 1;
    if (side == 1 && rayDirY < 0)
      texX = TEXT_WIDTH - texX - 1; // TEXT_WIDTH not TEXT_HEIGHT

    // How much to increase the texture coordinate per screen pixel
    double step = 1.0 * TEXT_HEIGHT / lineHeight;

    // Starting texture coordinate
    double texPos =
        (drawStart - player->pitch - game->window_height / 2 + lineHeight / 2) *
        step;

    // Draw the textured vertical line
    for (int y = drawStart; y < drawEnd; y++) {
      int texY = (int)texPos & (TEXT_HEIGHT - 1);
      texPos += step;

      uint32_t color = tm->textures[texNum][TEXT_WIDTH * texY + texX];

      if (side == 1)
        color = (color >> 1) & 8355711;

      game->buffer[y * game->window_width + x] = color;
    }
    // set z-buffer for sprites
    game->Zbuffer[x] = perpWallDist;
  }
}

void perform_floorcasting(Engine *engine) {
  // FLOOR CASTING
  for (int y = 0; y < game->window_height; y++) {
    // rayDir for leftmost ray (x = 0) and rightmost ray (x = w)
    float rayDirX0 = player->dirX - player->planeX;
    float rayDirY0 = player->dirY - player->planeY;
    float rayDirX1 = player->dirX + player->planeX;
    float rayDirY1 = player->dirY + player->planeY;

    // Current y position compared to the center of the screen (the horizon)
    int p = y - player->pitch - game->window_height / 2;
    if (p == 0)
      continue;

    // Vertical position of the camera.
    float posZ = 0.5 * game->window_height;

    // Horizontal distance from the camera to the floor for the current row.
    // 0.5 is the z position exactly in the middle between floor and ceiling.
    float rowDistance = posZ / abs(p);

    // calculate the real world step vector we have to add for each x (parallel
    // to camera plane) adding step by step avoids multiplications with a weight
    // in the inner loop
    float floorStepX = rowDistance * (rayDirX1 - rayDirX0) / game->window_width;
    float floorStepY = rowDistance * (rayDirY1 - rayDirY0) / game->window_width;

    // real world coordinates of the leftmost column. This will be updated as we
    // step to the right.
    float floorX = player->posX + rowDistance * rayDirX0;
    float floorY = player->posY + rowDistance * rayDirY0;

    for (int x = 0; x < game->window_width; ++x) {
      // the cell coord is simply got from the integer parts of floorX and
      // floorY
      int cellX = (int)(floorX);
      int cellY = (int)(floorY);

      // get the texture coordinate from the fractional part
      int tx = (int)(TEXT_WIDTH * (floorX - cellX)) & (TEXT_WIDTH - 1);
      int ty = (int)(TEXT_HEIGHT * (floorY - cellY)) & (TEXT_HEIGHT - 1);

      floorX += floorStepX;
      floorY += floorStepY;

      // choose texture and draw the pixel
      int floorTexture = 3;
      int ceilingTexture = 6;
      Uint32 color;

      if (p > 0) {
        // floor (below horizon)
        color = tm->textures[floorTexture][TEXT_WIDTH * ty + tx];
        color = (color >> 1) & 8355711;
        game->buffer[y * game->window_width + x] = color;
      } else {
        // ceiling (above horizon)
        color = tm->textures[ceilingTexture][TEXT_WIDTH * ty + tx];
        color = (color >> 1) & 8355711;
        game->buffer[y * game->window_width + x] = color;
      }
    }
  }
}
