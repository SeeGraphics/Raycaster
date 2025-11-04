#include "lights.h"
#include "entities.h"
#include "map.h"
#include <math.h>
#include <string.h>

#define LIGHTS_MAX_COUNT 64
#define LIGHTS_AMBIENT 0.18f

typedef struct
{
  double x;
  double y;
  float radius;
  float intensity;
  float colorR;
  float colorG;
  float colorB;
} Light;

static Light g_lights[LIGHTS_MAX_COUNT];
static int g_lightCount = 0;

static float g_lightLevel[MAP_WIDTH][MAP_HEIGHT];
static float g_lightColorR[MAP_WIDTH][MAP_HEIGHT];
static float g_lightColorG[MAP_WIDTH][MAP_HEIGHT];
static float g_lightColorB[MAP_WIDTH][MAP_HEIGHT];

static inline float clamp01(float v)
{
  if (v < 0.0f)
    return 0.0f;
  if (v > 1.0f)
    return 1.0f;
  return v;
}

void lights_reset(void)
{
  g_lightCount = 0;
  memset(g_lightLevel, 0, sizeof(g_lightLevel));
  memset(g_lightColorR, 0, sizeof(g_lightColorR));
  memset(g_lightColorG, 0, sizeof(g_lightColorG));
  memset(g_lightColorB, 0, sizeof(g_lightColorB));
}

void lights_add(double x, double y, float radius, float intensity,
                float colorR, float colorG, float colorB)
{
  if (g_lightCount >= LIGHTS_MAX_COUNT)
    return;
  if (radius <= 0.0f || intensity <= 0.0f)
    return;

  Light *light = &g_lights[g_lightCount++];
  light->x = x;
  light->y = y;
  light->radius = radius;
  light->intensity = intensity;
  light->colorR = clamp01(colorR);
  light->colorG = clamp01(colorG);
  light->colorB = clamp01(colorB);
}

void lights_buildMap(void)
{
  for (int x = 0; x < MAP_WIDTH; ++x)
  {
    for (int y = 0; y < MAP_HEIGHT; ++y)
    {
      g_lightLevel[x][y] = LIGHTS_AMBIENT;
      g_lightColorR[x][y] = LIGHTS_AMBIENT;
      g_lightColorG[x][y] = LIGHTS_AMBIENT;
      g_lightColorB[x][y] = LIGHTS_AMBIENT;
    }
  }

  for (int i = 0; i < g_lightCount; ++i)
  {
    const Light *light = &g_lights[i];
    for (int x = 0; x < MAP_WIDTH; ++x)
    {
      for (int y = 0; y < MAP_HEIGHT; ++y)
      {
        double tileCenterX = (double)x + 0.5;
        double tileCenterY = (double)y + 0.5;
        double dx = tileCenterX - light->x;
        double dy = tileCenterY - light->y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist > light->radius)
          continue;

        float t = (float)(1.0 - (dist / light->radius));
        if (t <= 0.0f)
          continue;
        float contrib = light->intensity * t * t;

        if (contrib <= 0.0f)
          continue;

        float colorR = light->colorR;
        float colorG = light->colorG;
        float colorB = light->colorB;
        float colorSum = colorR + colorG + colorB;
        if (colorSum <= 0.0f)
        {
          colorR = colorG = colorB = 1.0f;
          colorSum = 3.0f;
        }
        float normR = colorR / colorSum;
        float normG = colorG / colorSum;
        float normB = colorB / colorSum;

        g_lightColorR[x][y] += contrib * normR;
        g_lightColorG[x][y] += contrib * normG;
        g_lightColorB[x][y] += contrib * normB;
        g_lightLevel[x][y] += contrib;
      }
    }
  }

  for (int x = 0; x < MAP_WIDTH; ++x)
  {
    for (int y = 0; y < MAP_HEIGHT; ++y)
    {
      g_lightColorR[x][y] = clamp01(g_lightColorR[x][y]);
      g_lightColorG[x][y] = clamp01(g_lightColorG[x][y]);
      g_lightColorB[x][y] = clamp01(g_lightColorB[x][y]);
      float maxChannel = g_lightColorR[x][y];
      if (g_lightColorG[x][y] > maxChannel)
        maxChannel = g_lightColorG[x][y];
      if (g_lightColorB[x][y] > maxChannel)
        maxChannel = g_lightColorB[x][y];
      g_lightLevel[x][y] = clamp01(maxChannel);
    }
  }
}

void lights_sampleTile(int tileX, int tileY, float *outBrightness,
                       float *outR, float *outG, float *outB)
{
  if (tileX < 0 || tileX >= MAP_WIDTH || tileY < 0 || tileY >= MAP_HEIGHT)
  {
    if (outBrightness)
      *outBrightness = LIGHTS_AMBIENT;
    if (outR)
      *outR = LIGHTS_AMBIENT;
    if (outG)
      *outG = LIGHTS_AMBIENT;
    if (outB)
      *outB = LIGHTS_AMBIENT;
    return;
  }

  if (outBrightness)
    *outBrightness = g_lightLevel[tileX][tileY];
  if (outR)
    *outR = g_lightColorR[tileX][tileY];
  if (outG)
    *outG = g_lightColorG[tileX][tileY];
  if (outB)
    *outB = g_lightColorB[tileX][tileY];
}

void lights_sampleWorld(double x, double y, float *outBrightness,
                        float *outR, float *outG, float *outB)
{
  int x0 = (int)floor(x);
  int y0 = (int)floor(y);
  int x1 = x0 + 1;
  int y1 = y0 + 1;
  float fx = (float)(x - x0);
  float fy = (float)(y - y0);

  float b00, b10, b01, b11;
  float r00, r10, r01, r11;
  float g00, g10, g01, g11;
  float bcol00, bcol10, bcol01, bcol11;

  lights_sampleTile(x0, y0, &b00, &r00, &g00, &bcol00);
  lights_sampleTile(x1, y0, &b10, &r10, &g10, &bcol10);
  lights_sampleTile(x0, y1, &b01, &r01, &g01, &bcol01);
  lights_sampleTile(x1, y1, &b11, &r11, &g11, &bcol11);

  float wx0 = 1.0f - fx;
  float wy0 = 1.0f - fy;

  float brightness =
      b00 * wx0 * wy0 + b10 * fx * wy0 + b01 * wx0 * fy + b11 * fx * fy;
  float colR = r00 * wx0 * wy0 + r10 * fx * wy0 + r01 * wx0 * fy + r11 * fx * fy;
  float colG = g00 * wx0 * wy0 + g10 * fx * wy0 + g01 * wx0 * fy + g11 * fx * fy;
  float colB =
      bcol00 * wx0 * wy0 + bcol10 * fx * wy0 + bcol01 * wx0 * fy + bcol11 * fx * fy;

  if (outBrightness)
    *outBrightness = brightness;
  if (outR)
    *outR = colR;
  if (outG)
    *outG = colG;
  if (outB)
    *outB = colB;
}

u32 lights_applyColor(u32 color, float brightness, float tintR, float tintG,
                      float tintB)
{
  (void)brightness;
  float rFactor = clamp01(tintR);
  float gFactor = clamp01(tintG);
  float bFactor = clamp01(tintB);

  u32 alpha = (color >> 24) & 0xFFu;
  float r = (float)((color >> 16) & 0xFFu) * rFactor;
  float g = (float)((color >> 8) & 0xFFu) * gFactor;
  float b = (float)(color & 0xFFu) * bFactor;

  if (r > 255.0f)
    r = 255.0f;
  if (g > 255.0f)
    g = 255.0f;
  if (b > 255.0f)
    b = 255.0f;

  if (r < 0.0f)
    r = 0.0f;
  if (g < 0.0f)
    g = 0.0f;
  if (b < 0.0f)
    b = 0.0f;

  u32 outR = (u32)(r + 0.5f);
  u32 outG = (u32)(g + 0.5f);
  u32 outB = (u32)(b + 0.5f);

  return (alpha << 24) | (outR << 16) | (outG << 8) | outB;
}
