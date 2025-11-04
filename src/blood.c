#include "blood.h"
#include "engine.h"
#include "player.h"
#include "types.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BLOOD_MAX_PARTICLES 256

typedef struct
{
  double x;
  double y;
  double height;
  double vx;
  double vy;
  double vz;
  double life;
  double age;
  int pixelSize;
  u32 color;
} BloodParticle;

static BloodParticle g_bloodParticles[BLOOD_MAX_PARTICLES];

static double blood_rand_unit(void)
{
  return (double)rand() / (double)RAND_MAX;
}

void blood_reset(void)
{
  memset(g_bloodParticles, 0, sizeof(g_bloodParticles));
}

static void blood_spawn_particle(double x, double y, double dirX, double dirY)
{
  for (int i = 0; i < BLOOD_MAX_PARTICLES; ++i)
  {
    BloodParticle *p = &g_bloodParticles[i];
    if (p->age >= p->life)
    {
      double forwardSpeed = 0.4 + blood_rand_unit() * 0.8;
      double lateralAmount = (blood_rand_unit() - 0.5)   * 35.0;
      double perpX = -dirY;
      double perpY = dirX;
      double jitterX = (blood_rand_unit() - 0.5) * 0.5;
      double jitterY = (blood_rand_unit() - 0.5) * 0.5;
      p->x = x + dirX * 0.2 + jitterX;
      p->y = y + dirY * 0.2 + jitterY;
      p->height = 1.1 + blood_rand_unit() * 0.7;
      p->vx = dirX * forwardSpeed + perpX * lateralAmount;
      p->vy = dirY * forwardSpeed + perpY * lateralAmount;
      p->vz = 1.2 + blood_rand_unit() * 1.6;
      p->life = 0.45 + blood_rand_unit() * 0.55;
      p->age = 0.0;
      p->pixelSize = 3 + rand() % 3;
      int baseR = 170 + (int)(blood_rand_unit()  * 35.0);
      int baseG = 8 + (int)(blood_rand_unit() * 24.0);
      int baseB = 8 + (int)(blood_rand_unit() * 20.0);
      p->color = (0xFFu << 24) | ((u32)baseR << 16) | ((u32)baseG << 8) | (u32)baseB;
      return;
    }
  }
}

void blood_spawnBurst(double x, double y, double dirX, double dirY)
{
  const int count = 16;
  double len = sqrt(dirX * dirX + dirY * dirY);
  if (len < 1e-6)
  {
    dirX = 1.0;
    dirY = 0.0;
  }
  else
  {
    dirX /= len;
    dirY /= len;
  }
  for (int i = 0; i < count; ++i)
    blood_spawn_particle(x, y, dirX, dirY);
}

void blood_update(double deltaTime)
{
  if (deltaTime <= 0.0)
    return;

  for (int i = 0; i < BLOOD_MAX_PARTICLES; ++i)
  {
    BloodParticle *p = &g_bloodParticles[i];
    if (p->age >= p->life)
      continue;

    p->age += deltaTime;
    if (p->age >= p->life)
      continue;

    p->x += p->vx * deltaTime;
    p->y += p->vy * deltaTime;
    p->height += p->vz * deltaTime;
    p->vz -= 9.0 * deltaTime;
    p->vx *= 0.94;
    p->vy *= 0.94;
    if (p->height <= 0.0)
    {
      p->height = 0.0;
      if (p->vz < 0.0)
        p->vz = 0.0;
      p->vx *= 0.78;
      p->vy *= 0.78;
    }
  }
}

void blood_render(struct Engine *engine)
{
  if (!engine || !engine->game.Rbuffer)
    return;

  Player *player = &engine->player;
  double invDet =
      1.0 / (player->planeX * player->dirY - player->dirX * player->planeY);

  const int width = RENDER_WIDTH;
  const int height = RENDER_HEIGHT;

  for (int i = 0; i < BLOOD_MAX_PARTICLES; ++i)
  {
    BloodParticle *p = &g_bloodParticles[i];
    if (p->age >= p->life)
      continue;

    double dx = p->x - player->posX;
    double dy = p->y - player->posY;

    double transformX = invDet * (player->dirY * dx - player->dirX * dy);
    double transformY = invDet * (-player->planeY * dx + player->planeX * dy);
    if (transformY <= 0.1)
      continue;

    int spriteScreenX =
        (int)((width / 2) * (1 + transformX / transformY));

    int spriteHeight = (int)(height / transformY);
    int centerY = height / 2 + (int)player->pitch + spriteHeight;
    int centerX = spriteScreenX;

    if (p->height > 0.0)
    {
      int lift = (int)((p->height * height) / transformY);
      centerY -= lift;
    }

    int drawSize = (int)fmax(1.0, 3.375 * (double)p->pixelSize / transformY);
    if (drawSize > 12)
      drawSize = 12;

    int half = drawSize / 2;
    int top = centerY - half;
    int left = centerX - half;
    int bottom = top + drawSize;
    int right = left + drawSize;

    if (right < 0 || left >= width || bottom < 0 || top >= height)
      continue;

    double fade = 1.0 - (p->age / p->life);
    if (fade < 0.0)
      fade = 0.0;

    u32 color = p->color;
    int baseR = (color >> 16) & 0xFF;
    int baseG = (color >> 8) & 0xFF;
    int baseB = color & 0xFF;
    int finalR = (int)(baseR * fade);
    int finalG = (int)(baseG * fade);
    int finalB = (int)(baseB * fade);
    if (finalR < 0)
      finalR = 0;
    if (finalG < 0)
      finalG = 0;
    if (finalB < 0)
      finalB = 0;
    u32 finalColor = (0xFFu << 24) | ((u32)finalR << 16) | ((u32)finalG << 8) | (u32)finalB;

    int drawStartY = top < 0 ? 0 : top;
    int drawEndY = bottom >= height ? height - 1 : bottom;
    int drawStartX = left < 0 ? 0 : left;
    int drawEndX = right >= width ? width - 1 : right;

    for (int y = drawStartY; y <= drawEndY; ++y)
    {
      int rowOffset = y * width;
      for (int x = drawStartX; x <= drawEndX; ++x)
      {
        if (transformY >= engine->game.Zbuffer[x])
          continue;
        engine->game.Rbuffer[rowOffset + x] = finalColor;
      }
    }
  }
}
