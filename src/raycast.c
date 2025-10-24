#include "raycast.h"
#include "engine.h"
#include "map.h"

void perform_raycasting(Engine *engine) {
  for (int x = 0; x < RENDER_WIDTH; x++) {
    double cameraX = 2 * x / (double)RENDER_WIDTH - 1;
    double rayDirX = engine->player.dirX + engine->player.planeX * cameraX;
    double rayDirY = engine->player.dirY + engine->player.planeY * cameraX;

    int mapX = (int)engine->player.posX;
    int mapY = (int)engine->player.posY;

    double deltaDistX = (rayDirX == 0) ? 1e30 : fabs(1.0 / rayDirX);
    double deltaDistY = (rayDirY == 0) ? 1e30 : fabs(1.0 / rayDirY);

    double sideDistX = (rayDirX < 0)
                           ? (engine->player.posX - mapX) * deltaDistX
                           : (mapX + 1.0 - engine->player.posX) * deltaDistX;
    double sideDistY = (rayDirY < 0)
                           ? (engine->player.posY - mapY) * deltaDistY
                           : (mapY + 1.0 - engine->player.posY) * deltaDistY;

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
    int lineHeight = (int)(RENDER_HEIGHT / perpWallDist);

    int drawStart =
        -lineHeight / 2 + RENDER_HEIGHT / 2 + (int)engine->player.pitch;
    if (drawStart < 0)
      drawStart = 0;

    int drawEnd =
        lineHeight / 2 + RENDER_HEIGHT / 2 + (int)engine->player.pitch;
    if (drawEnd >= RENDER_HEIGHT)
      drawEnd = RENDER_HEIGHT - 1;

    // texturing math
    int texNum = worldMap[mapX][mapY] - 1;

    // calculate value of wallX
    double wallX;
    if (side == 0)
      wallX = engine->player.posY + perpWallDist * rayDirY;
    else
      wallX = engine->player.posX + perpWallDist * rayDirX;
    wallX -= floor(wallX);

    // x coordinate on the texture
    int texX = (int)(wallX * (double)TEXT_WIDTH);
    if (side == 0 && rayDirX > 0)
      texX = TEXT_WIDTH - texX - 1;
    if (side == 1 && rayDirY < 0)
      texX = TEXT_WIDTH - texX - 1;

    // How much to increase the texture coordinate per screen pixel
    double step = 1.0 * TEXT_HEIGHT / lineHeight;

    // Starting texture coordinate
    double texPos = (drawStart - (int)engine->player.pitch - RENDER_HEIGHT / 2 +
                     lineHeight / 2) *
                    step;

    // Draw the textured vertical line
    for (int y = drawStart; y < drawEnd; y++) {
      int texY = (int)texPos & (TEXT_HEIGHT - 1);
      texPos += step;

      uint32_t color =
          engine->textures.textures[texNum][texY * TEXT_WIDTH + texX];

      if (side == 1)
        color = (color >> 1) & 8355711;

      engine->game.Rbuffer[y * RENDER_WIDTH + x] = color;
    }
    // set z-buffer for sprites
    engine->game.Zbuffer[x] = perpWallDist;
  }
}

void perform_floorcasting(Engine *engine) {
  // FLOOR CASTING
  for (int y = 0; y < RENDER_HEIGHT; y++) {
    // rayDir for leftmost ray (x = 0) and rightmost ray (x = w)
    float rayDirX0 = engine->player.dirX - engine->player.planeX;
    float rayDirY0 = engine->player.dirY - engine->player.planeY;
    float rayDirX1 = engine->player.dirX + engine->player.planeX;
    float rayDirY1 = engine->player.dirY + engine->player.planeY;

    // Current y position compared to the center of the screen (the horizon)
    int p = y - (int)engine->player.pitch - RENDER_HEIGHT / 2;
    if (p == 0)
      continue;

    // Vertical position of the camera
    float posZ = 0.5 * RENDER_HEIGHT;

    // Horizontal distance from the camera to the floor for the current row
    float rowDistance = posZ / abs(p);

    // Calculate the real world step vector
    float floorStepX = rowDistance * (rayDirX1 - rayDirX0) / RENDER_WIDTH;
    float floorStepY = rowDistance * (rayDirY1 - rayDirY0) / RENDER_WIDTH;

    // Real world coordinates of the leftmost column
    float floorX = engine->player.posX + rowDistance * rayDirX0;
    float floorY = engine->player.posY + rowDistance * rayDirY0;

    for (int x = 0; x < RENDER_WIDTH; ++x) {
      // Get cell coordinates
      int cellX = (int)(floorX);
      int cellY = (int)(floorY);

      // Get texture coordinate from the fractional part
      int tx = (int)(TEXT_WIDTH * (floorX - cellX)) & (TEXT_WIDTH - 1);
      int ty = (int)(TEXT_HEIGHT * (floorY - cellY)) & (TEXT_HEIGHT - 1);

      floorX += floorStepX;
      floorY += floorStepY;

      // Choose texture and draw the pixel
      int floorTexture = 3;
      int ceilingTexture = 6;
      uint32_t color;

      if (p > 0) {
        // Floor (below horizon)
        color = engine->textures.textures[floorTexture][TEXT_WIDTH * ty + tx];
        color = (color >> 1) & 8355711;
        engine->game.Rbuffer[y * RENDER_WIDTH + x] = color;
      } else {
        // Ceiling (above horizon)
        color = engine->textures.textures[ceilingTexture][TEXT_WIDTH * ty + tx];
        color = (color >> 1) & 8355711;
        engine->game.Rbuffer[y * RENDER_WIDTH + x] = color;
      }
    }
  }
}
