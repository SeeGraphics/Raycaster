#include "raycast.h"
#include "engine.h"
#include "map.h"
#include "entities.h"

int g_floorTextureId = 3;
int g_ceilingTextureId = 6;

void perform_raycasting(Engine *engine)
{
  for (int x = 0; x < RENDER_WIDTH; x++)
  {
    // map x coordinates
    double cameraX = 2 * x / (double)RENDER_WIDTH - 1;

    // ray calculation with camera plane and column
    double rayDirX = engine->player.dirX + engine->player.planeX * cameraX;
    double rayDirY = engine->player.dirY + engine->player.planeY * cameraX;

    // map player pos to mapX and mapY
    int mapX = (int)engine->player.posX;
    int mapY = (int)engine->player.posY;

    // DDA math
    double deltaDistX = (rayDirX == 0) ? 1e30 : fabs(1.0 / rayDirX);
    double deltaDistY = (rayDirY == 0) ? 1e30 : fabs(1.0 / rayDirY);

    double sideDistX = (rayDirX < 0)
                           ? (engine->player.posX - mapX) * deltaDistX
                           : (mapX + 1.0 - engine->player.posX) * deltaDistX;
    double sideDistY = (rayDirY < 0)
                           ? (engine->player.posY - mapY) * deltaDistY
                           : (mapY + 1.0 - engine->player.posY) * deltaDistY;

    // either step in left or right direction
    int stepX = (rayDirX < 0) ? -1 : 1;
    int stepY = (rayDirY < 0) ? -1 : 1;

    int hit = 0;
    int side;

    while (!hit)
    {
      if (sideDistX < sideDistY)
      {
        sideDistX += deltaDistX; // move to next horizontal grid line
        mapX += stepX;
        side = 0; // vertical wall hit (NS)
      }
      else
      {
        sideDistY += deltaDistY; // move to next vertical grid line
        mapY += stepY;
        side = 1; // horizontal wall hit (EW)
      }

      if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT &&
          worldMap[mapX][mapY] > 0)
      {
        hit = 1;
      }
    }

    // calculate perpendicular walldist (no fisheye effect)
    double perpWallDist =
        (side == 0) ? sideDistX - deltaDistX : sideDistY - deltaDistY;
    // wall size
    int lineHeight = (int)(RENDER_HEIGHT / perpWallDist);

    int drawStart =
        -lineHeight / 2 + RENDER_HEIGHT / 2 + (int)engine->player.pitch;
    if (drawStart < 0)
      drawStart = 0;

    int drawEnd =
        lineHeight / 2 + RENDER_HEIGHT / 2 + (int)engine->player.pitch;
    if (drawEnd >= RENDER_HEIGHT)
      drawEnd = RENDER_HEIGHT - 1;

    // texturing
    // get texture index in map array (-1 so we can use texture 0 as air)
    int texNum = worldMap[mapX][mapY] - 1;

    // calculate value of wallX
    double wallX;
    if (side == 0)
      wallX = engine->player.posY + perpWallDist * rayDirY;
    else
      wallX = engine->player.posX + perpWallDist * rayDirX;
    wallX -= floor(wallX);

    // x coordinate on the texture
    // flip texture depending on direction to not appear mirrored
    int texX = (int)(wallX * (double)TEXT_WIDTH);
    if (side == 0 && rayDirX > 0)
      texX = TEXT_WIDTH - texX - 1;
    if (side == 1 && rayDirY < 0)
      texX = TEXT_WIDTH - texX - 1;

    // Vertical texture Sampling
    // How much to increase the texture coordinate per screen pixel
    double step = 1.0 * TEXT_HEIGHT / lineHeight;

    // Starting texture coordinate
    double texPos = (drawStart - (int)engine->player.pitch - RENDER_HEIGHT / 2 +
                     lineHeight / 2) *
                    step;

    int faceX = 0;
    int faceY = 0;
    if (side == 0)
      faceX = -stepX;
    else
      faceY = -stepY;

    // Draw the textured vertical line
    for (int y = drawStart; y < drawEnd; y++)
    {
      int texY = (int)texPos & (TEXT_HEIGHT - 1);
      texPos += step;

      u32 color = engine->textures.textures[texNum][texY * TEXT_WIDTH + texX];

      if (side == 1)
        color = (color >> 1) & 8355711;

      int leverTexIndex =
          entities_getLeverTextureAtFace(mapX, mapY, faceX, faceY, NULL);
      if (leverTexIndex >= 0)
      {
        const u32 *leverTex = engine->textures.textures[leverTexIndex];
        if (leverTex)
        {
          const float coverageX = 0.3f;
          const float coverageY = 0.35f;
          float u = ((float)texX + 0.5f) / (float)TEXT_WIDTH;
          float v = ((float)texY + 0.5f) / (float)TEXT_HEIGHT;
          float localU = (u - 0.5f) / coverageX + 0.5f;
          float localV = (v - 0.5f) / coverageY + 0.5f;
          if (localU >= 0.0f && localU <= 1.0f && localV >= 0.0f &&
              localV <= 1.0f)
          {
            int sampleX = (int)(localU * (float)(TEXT_WIDTH - 1));
            int sampleY = (int)(localV * (float)(TEXT_HEIGHT - 1));
            if (sampleX >= 0 && sampleX < TEXT_WIDTH && sampleY >= 0 &&
                sampleY < TEXT_HEIGHT)
            {
              u32 leverColor = leverTex[sampleY * TEXT_WIDTH + sampleX];
              if ((leverColor & 0xFF000000u) != 0)
                color = leverColor;
            }
          }
        }
      }

      engine->game.Rbuffer[y * RENDER_WIDTH + x] = color;
    }
    // set z-buffer for sprites
    engine->game.Zbuffer[x] = perpWallDist;
  }
}

void perform_floorcasting(Engine *engine)
{
  // FLOOR CASTING
  for (int y = 0; y < RENDER_HEIGHT; y++)
  {
    // rayDir for leftmost ray (x = 0) and rightmost ray (x = w)
    f32 rayDirX0 = engine->player.dirX - engine->player.planeX;
    f32 rayDirY0 = engine->player.dirY - engine->player.planeY;
    f32 rayDirX1 = engine->player.dirX + engine->player.planeX;
    f32 rayDirY1 = engine->player.dirY + engine->player.planeY;

    // Current y position compared to the center of the screen (the horizon)
    int p = y - (int)engine->player.pitch - RENDER_HEIGHT / 2;
    if (p == 0)
      continue;

    // Vertical position of the camera
    f32 posZ = 0.5 * RENDER_HEIGHT;

    // Horizontal distance from the camera to the floor for the current row
    f32 rowDistance = posZ / abs(p);

    // Calculate the real world step vector
    f32 floorStepX = rowDistance * (rayDirX1 - rayDirX0) / RENDER_WIDTH;
    f32 floorStepY = rowDistance * (rayDirY1 - rayDirY0) / RENDER_WIDTH;

    // Real world coordinates of the leftmost column
    f32 floorX = engine->player.posX + rowDistance * rayDirX0;
    f32 floorY = engine->player.posY + rowDistance * rayDirY0;

    for (int x = 0; x < RENDER_WIDTH; ++x)
    {
      // Get cell coordinates
      int cellX = (int)(floorX);
      int cellY = (int)(floorY);

      // Get texture coordinate from the fractional part
      int tx = (int)(TEXT_WIDTH * (floorX - cellX)) & (TEXT_WIDTH - 1);
      int ty = (int)(TEXT_HEIGHT * (floorY - cellY)) & (TEXT_HEIGHT - 1);

      floorX += floorStepX;
      floorY += floorStepY;

      // Choose texture and draw the pixel
      int floorTexture = g_floorTextureId;
      int ceilingTexture = g_ceilingTextureId;
      u32 color;

      if (p > 0)
      {
        // Floor (below horizon)
        color = engine->textures.textures[floorTexture][TEXT_WIDTH * ty + tx];
        color = (color >> 1) & 8355711;
        engine->game.Rbuffer[y * RENDER_WIDTH + x] = color;
      }
      else
      {
        // Ceiling (above horizon)
        color = engine->textures.textures[ceilingTexture][TEXT_WIDTH * ty + tx];
        color = (color >> 1) & 8355711;
        engine->game.Rbuffer[y * RENDER_WIDTH + x] = color;
      }
    }
  }
}
