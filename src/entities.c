#include "animation.h"
#include "entities.h"
#include "player.h"
#include "raycast.h"
#include "texture.h"
#include "map.h"
#include "engine.h"
#include "types.h"
#include <ctype.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Sprite worldSprites[NUM_SPRITES];
static i32 worldSpriteCount = 0;
static int worldInitialized = 0;

typedef struct
{
  const char *name;
  int x;
  int y;
} LeverFacingDef;

static const LeverFacingDef g_leverFacingDefs[] = {
    {"EAST", 1, 0},
    {"WEST", -1, 0},
    {"SOUTH", 0, 1},
    {"NORTH", 0, -1},
};
static const int g_leverFacingDefCount =
    (int)(sizeof(g_leverFacingDefs) / sizeof(g_leverFacingDefs[0]));

typedef struct
{
  int tileX;
  int tileY;
  int faceX;
  int faceY;
  float scale;
  u32 *pixels;
  float coverageX;
  float coverageY;
} WallTextInstance;

static WallTextInstance *g_wallTexts = NULL;
static int g_wallTextCount = 0;
static int g_wallTextCapacity = 0;

static float walltext_compute_final_height(int srcW, int srcH, int renderWidth,
                                           int renderHeight, float maxScale,
                                           float heightBias)
{
  if (srcW <= 0 || srcH <= 0)
    return 0.0f;

  float scaleX = ((float)renderWidth * maxScale) / (float)srcW;
  float scaleY = ((float)renderHeight * maxScale) / (float)srcH;
  float scale = (scaleX < scaleY) ? scaleX : scaleY;
  if (scale <= 0.0f)
    return 0.0f;

  scale *= heightBias;
  float maxScaleX = (float)renderWidth / (float)srcW;
  float maxScaleY = (float)renderHeight / (float)srcH;
  float maxScaleClamp = (maxScaleX < maxScaleY) ? maxScaleX : maxScaleY;
  if (scale > maxScaleClamp)
    scale = maxScaleClamp;

  return (float)srcH * scale;
}

typedef struct
{
  float interactX;
  float interactY;
  float leverX;
  float leverY;
  int doorX;
  int doorY;
  int tileX;
  int tileY;
  int offTextureId;
  int onTextureId;
  int activated;
  int normalX;
  int normalY;
  int openTileValue;
  int originalTileValue;
} LeverInstance;

static LeverInstance *g_levers = NULL;
static int g_leverCount = 0;
static int g_leverCapacity = 0;
static int g_leverDoorMap[MAP_WIDTH][MAP_HEIGHT];
static int g_leverTileMap[MAP_WIDTH][MAP_HEIGHT];

static double g_playerSpawnX = POS_X;
static double g_playerSpawnY = POS_Y;
static double g_playerSpawnDirDegrees = 0.0;

static double entities_calculateDirDegrees(double dirX, double dirY)
{
  double angle = atan2(-dirY, dirX) * (180.0 / M_PI);
  if (angle < 0.0)
    angle += 360.0;
  return angle;
}

static void entities_resetSpawnToDefaults(void)
{
  g_playerSpawnX = POS_X;
  g_playerSpawnY = POS_Y;
  g_playerSpawnDirDegrees = entities_calculateDirDegrees(DIR_X, DIR_Y);
}

static Animation *animation_from_name(const char *name);

typedef struct
{
  const char *jsonName;
  const char *offTextureName;
  const char *onTextureName;
} DecalDefinition;

static const DecalDefinition g_decalDefinitions[] = {
    {"LEVER", "LeverOff", "LeverOn"},
};
static const int g_decalDefinitionCount =
    (int)(sizeof(g_decalDefinitions) / sizeof(g_decalDefinitions[0]));

static int decal_definition_index(const char *jsonName)
{
  if (!jsonName)
    return -1;
  for (int i = 0; i < g_decalDefinitionCount; ++i)
  {
    if (strcmp(g_decalDefinitions[i].jsonName, jsonName) == 0)
      return i;
  }
  return -1;
}

static int clamp_int(int value, int minValue, int maxValue)
{
  if (value < minValue)
    return minValue;
  if (value > maxValue)
    return maxValue;
  return value;
}

static void walltext_clear(void)
{
  for (int i = 0; i < g_wallTextCount; ++i)
  {
    free(g_wallTexts[i].pixels);
    g_wallTexts[i].pixels = NULL;
  }
  g_wallTextCount = 0;
}

static void walltext_ensureCapacity(int required)
{
  if (required <= g_wallTextCapacity)
    return;
  int newCap = g_wallTextCapacity ? g_wallTextCapacity * 2 : 8;
  while (newCap < required)
    newCap *= 2;
  WallTextInstance *newData =
      (WallTextInstance *)realloc(g_wallTexts, newCap * sizeof(WallTextInstance));
  if (!newData)
    return;
  g_wallTexts = newData;
  g_wallTextCapacity = newCap;
}

static u32 *walltext_render_texture(const char *text, int fontSize,
                                    SDL_Color color, float *outCoverageX,
                                    float *outCoverageY)
{
  if (!text || text[0] == '\0')
    return NULL;

  if (outCoverageX)
    *outCoverageX = 1.0f;
  if (outCoverageY)
    *outCoverageY = 1.0f;

  const float maxScaleFrac = 0.92f;
  const float heightBias = 1.08f;
  const int upscale = 4;
  const int renderWidth = TEXT_WIDTH * upscale;
  const int renderHeight = TEXT_HEIGHT * upscale;

  TTF_Font *font = TTF_OpenFont("assets/font/Doom.ttf", fontSize);
  if (!font)
  {
    fprintf(stderr, "\033[31m[ERROR] Failed to load wall text font: %s\033[0m\n",
            TTF_GetError());
    return NULL;
  }

  SDL_Surface *baseSurface = TTF_RenderUTF8_Blended(font, text, color);
  if (!baseSurface)
  {
    fprintf(stderr, "\033[31m[ERROR] Failed to render wall text '%s': %s\033[0m\n",
            text, TTF_GetError());
    TTF_CloseFont(font);
    return NULL;
  }

  SDL_Surface *surface = baseSurface;
  float bestHeight = walltext_compute_final_height(surface->w, surface->h,
                                                   renderWidth, renderHeight,
                                                   maxScaleFrac, heightBias);

  if (strchr(text, ' ') != NULL)
  {
    int wrapWidth = surface->w;
    for (int attempt = 0; attempt < 6; ++attempt)
    {
      wrapWidth = (int)((float)wrapWidth * 0.85f);
      if (wrapWidth < fontSize)
        wrapWidth = fontSize;

      SDL_Surface *wrapped =
          TTF_RenderUTF8_Blended_Wrapped(font, text, color, wrapWidth);
      if (!wrapped)
        break;

      float candidateHeight = walltext_compute_final_height(
          wrapped->w, wrapped->h, renderWidth, renderHeight, maxScaleFrac,
          heightBias);

      if (candidateHeight > bestHeight)
      {
        if (surface != baseSurface)
          SDL_FreeSurface(surface);
        surface = wrapped;
        bestHeight = candidateHeight;
      }
      else
      {
        SDL_FreeSurface(wrapped);
      }

      if (wrapWidth == fontSize)
        break;
    }
  }

  if (surface != baseSurface)
    SDL_FreeSurface(baseSurface);

  SDL_Surface *converted =
      SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
  if (!converted)
  {
    fprintf(stderr, "\033[31m[ERROR] Failed to convert wall text surface: %s\033[0m\n",
            SDL_GetError());
    SDL_FreeSurface(surface);
    TTF_CloseFont(font);
    return NULL;
  }

  Uint32 *highBuffer =
      (Uint32 *)calloc((size_t)renderWidth * (size_t)renderHeight, sizeof(Uint32));
  if (!highBuffer)
  {
    SDL_FreeSurface(converted);
    SDL_FreeSurface(surface);
    TTF_CloseFont(font);
    return NULL;
  }

  Uint32 *srcPixels = (Uint32 *)converted->pixels;
  int srcPitch = converted->pitch / 4;
  int srcW = converted->w;
  int srcH = converted->h;
  if (srcW <= 0 || srcH <= 0)
  {
    free(highBuffer);
    SDL_FreeSurface(converted);
    SDL_FreeSurface(surface);
    TTF_CloseFont(font);
    return NULL;
  }

  float scaleX = ((float)renderWidth * maxScaleFrac) / (float)srcW;
  float scaleY = ((float)renderHeight * maxScaleFrac) / (float)srcH;
  float scale = (scaleX < scaleY) ? scaleX : scaleY;
  if (scale <= 0.0f)
    scale = ((float)renderHeight) / (float)srcH;

  scale *= heightBias;
  float maxScaleX = (float)renderWidth / (float)srcW;
  float maxScaleY = (float)renderHeight / (float)srcH;
  float maxScaleClamp = (maxScaleX < maxScaleY) ? maxScaleX : maxScaleY;
  if (scale > maxScaleClamp)
    scale = maxScaleClamp;

  int destWidth = (int)roundf((float)srcW * scale);
  int destHeight = (int)roundf((float)srcH * scale);
  if (destWidth < 1)
    destWidth = 1;
  if (destHeight < 1)
    destHeight = 1;
  if (destWidth > renderWidth)
    destWidth = renderWidth;
  if (destHeight > renderHeight)
    destHeight = renderHeight;

  int offsetX = (renderWidth - destWidth) / 2;
  int offsetY = (renderHeight - destHeight) / 2;

  for (int y = 0; y < destHeight; ++y)
  {
    int srcY = (srcH > 0) ? (y * srcH) / destHeight : 0;
    if (srcY >= srcH)
      srcY = srcH - 1;
    for (int x = 0; x < destWidth; ++x)
    {
      int srcX = (srcW > 0) ? (x * srcW) / destWidth : 0;
      if (srcX >= srcW)
        srcX = srcW - 1;
      Uint32 srcPixel = srcPixels[srcY * srcPitch + srcX];
      Uint8 r, g, b, a;
      SDL_GetRGBA(srcPixel, converted->format, &r, &g, &b, &a);
      if (a == 0)
        continue;
      highBuffer[(size_t)(y + offsetY) * (size_t)renderWidth +
                 (size_t)(x + offsetX)] =
          ((Uint32)a << 24) | ((Uint32)r << 16) | ((Uint32)g << 8) | (Uint32)b;
    }
  }

  u32 *buffer =
      (u32 *)calloc((size_t)TEXT_WIDTH * (size_t)TEXT_HEIGHT, sizeof(u32));
  if (!buffer)
  {
    free(highBuffer);
    SDL_FreeSurface(converted);
    SDL_FreeSurface(surface);
    TTF_CloseFont(font);
    return NULL;
  }

  for (int y = 0; y < TEXT_HEIGHT; ++y)
  {
    int srcY0 = y * upscale;
    int srcY1 = srcY0 + upscale;
    for (int x = 0; x < TEXT_WIDTH; ++x)
    {
      int srcX0 = x * upscale;
      int srcX1 = srcX0 + upscale;

      unsigned int sumA = 0;
      unsigned int sumR = 0;
      unsigned int sumG = 0;
      unsigned int sumB = 0;

      for (int sy = srcY0; sy < srcY1; ++sy)
      {
        for (int sx = srcX0; sx < srcX1; ++sx)
        {
          Uint32 pix =
              highBuffer[(size_t)sy * (size_t)renderWidth + (size_t)sx];
          Uint8 a = (Uint8)((pix >> 24) & 0xFFu);
          if (a == 0)
            continue;
          Uint8 r = (Uint8)((pix >> 16) & 0xFFu);
          Uint8 g = (Uint8)((pix >> 8) & 0xFFu);
          Uint8 b = (Uint8)(pix & 0xFFu);
          sumA += a;
          sumR += (unsigned int)r * (unsigned int)a;
          sumG += (unsigned int)g * (unsigned int)a;
          sumB += (unsigned int)b * (unsigned int)a;
        }
      }

      unsigned int sampleCount = (unsigned int)(upscale * upscale);
      Uint8 outA = (Uint8)(sumA / sampleCount);
      Uint8 outR = 0;
      Uint8 outG = 0;
      Uint8 outB = 0;
      if (sumA > 0)
      {
        outR = (Uint8)(sumR / sumA);
        outG = (Uint8)(sumG / sumA);
        outB = (Uint8)(sumB / sumA);
      }

      buffer[(size_t)y * (size_t)TEXT_WIDTH + (size_t)x] =
          ((Uint32)outA << 24) | ((Uint32)outR << 16) |
          ((Uint32)outG << 8) | (Uint32)outB;
    }
  }

  if (outCoverageX)
    *outCoverageX = (float)destWidth / (float)renderWidth;
  if (outCoverageY)
    *outCoverageY = (float)destHeight / (float)renderHeight;

  free(highBuffer);
  SDL_FreeSurface(converted);
  SDL_FreeSurface(surface);
  TTF_CloseFont(font);

  return buffer;
}
static void lever_ensureCapacity(int required)
{
  if (required <= g_leverCapacity)
    return;
  int newCap = g_leverCapacity ? g_leverCapacity * 2 : 8;
  while (newCap < required)
    newCap *= 2;
  LeverInstance *newData = (LeverInstance *)realloc(g_levers, newCap * sizeof(LeverInstance));
  if (!newData)
    return;
  g_levers = newData;
  g_leverCapacity = newCap;
}

static int lever_equalsIgnoreCase(const char *a, const char *b)
{
  while (*a && *b)
  {
    if (toupper((unsigned char)*a) != toupper((unsigned char)*b))
      return 0;
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

static int lever_parseFacing(const char *value, int *outX, int *outY)
{
  if (!value || !outX || !outY)
    return -1;

  for (int i = 0; i < g_leverFacingDefCount; ++i)
  {
    const LeverFacingDef *def = &g_leverFacingDefs[i];
    if (lever_equalsIgnoreCase(value, def->name))
    {
      *outX = def->x;
      *outY = def->y;
      return i;
    }
    if (value[0] != '\0' && value[1] == '\0')
    {
      char abbrev[2] = {def->name[0], '\0'};
      if (lever_equalsIgnoreCase(value, abbrev))
      {
        *outX = def->x;
        *outY = def->y;
        return i;
      }
    }
  }

  return -1;
}

static void lever_clear(void)
{
  g_leverCount = 0;
  for (int x = 0; x < MAP_WIDTH; ++x)
    for (int y = 0; y < MAP_HEIGHT; ++y)
    {
      g_leverDoorMap[x][y] = -1;
      g_leverTileMap[x][y] = -1;
    }
}

static void lever_alignPosition(LeverInstance *lever)
{
  if (!lever)
    return;
  static const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
  if (lever->tileX < 0 || lever->tileY < 0 || lever->tileX >= MAP_WIDTH ||
      lever->tileY >= MAP_HEIGHT)
    return;

  float leverCenterX = (float)lever->tileX + 0.5f;
  float leverCenterY = (float)lever->tileY + 0.5f;
  lever->leverX = leverCenterX;
  lever->leverY = leverCenterY;

  int dirX = lever->normalX;
  int dirY = lever->normalY;

  if (dirX == 0 && dirY == 0)
  {
    for (int i = 0; i < 4; ++i)
    {
      int nx = lever->tileX + dirs[i][0];
      int ny = lever->tileY + dirs[i][1];
      if (nx < 0 || ny < 0 || nx >= MAP_WIDTH || ny >= MAP_HEIGHT)
        continue;
      if (worldMap[nx][ny] != 0)
        continue;
      dirX = dirs[i][0];
      dirY = dirs[i][1];
      break;
    }
  }

  if (dirX > 1)
    dirX = 1;
  if (dirX < -1)
    dirX = -1;
  if (dirY > 1)
    dirY = 1;
  if (dirY < -1)
    dirY = -1;

  lever->normalX = dirX;
  lever->normalY = dirY;

  if (dirX == 0 && dirY == 0)
  {
    lever->interactX = leverCenterX;
    lever->interactY = leverCenterY;
    return;
  }

  float offset = 0.45f;
  lever->interactX = leverCenterX + dirX * offset;
  lever->interactY = leverCenterY + dirY * offset;

  lever->tileX = (int)floor(lever->leverX);
  lever->tileY = (int)floor(lever->leverY);
}

static Sprite sprite_make(f64 x, f64 y, SpriteKind kind,
                          SpriteAppearance appearance, f32 scale, i32 health)
{
  Sprite sprite;
  sprite.x = x;
  sprite.y = y;
  sprite.scale = scale;
  sprite.kind = kind;
  sprite.appearance = appearance;
  sprite.active = 1;
  sprite.health = health;
  sprite.auxTextureId = -1;
  sprite.targetX = -1;
  sprite.targetY = -1;
  sprite.actionType = 0;
  sprite.stateFlags = 0;
  return sprite;
}

static Sprite *entities_pushSprite(Sprite sprite)
{
  if (worldSpriteCount >= NUM_SPRITES)
    return NULL;

  worldSprites[worldSpriteCount] = sprite;
  return &worldSprites[worldSpriteCount++];
}

static void fill_unused_slots(void)
{
  for (i32 i = worldSpriteCount; i < NUM_SPRITES; ++i)
  {
    worldSprites[i].x = 0.0;
    worldSprites[i].y = 0.0;
    worldSprites[i].scale = 1.0f;
    worldSprites[i].kind = SPRITE_DECORATION;
    worldSprites[i].appearance = spriteAppearanceFromTexture(TEX_GREENLIGHT);
    worldSprites[i].active = 0;
    worldSprites[i].health = 0;
    worldSprites[i].auxTextureId = -1;
    worldSprites[i].targetX = -1;
    worldSprites[i].targetY = -1;
    worldSprites[i].actionType = 0;
    worldSprites[i].stateFlags = 0;
  }
}

static void entities_populateDefaults(void)
{
  worldSpriteCount = 0;
  entities_resetSpawnToDefaults();
  lever_clear();
  walltext_clear();

  static const struct
  {
    f32 x, y, scale;
    i32 textureId;
  } decorations[] = {
      {4.5f, 3.5f, 1.0f, TEX_GREENLIGHT},  {8.5f, 4.5f, 1.0f, TEX_GREENLIGHT},
      {15.5f, 4.5f, 1.0f, TEX_GREENLIGHT}, {18.5f, 4.5f, 1.0f, TEX_GREENLIGHT},
      {7.5f, 8.5f, 1.0f, TEX_GREENLIGHT},  {7.5f, 16.5f, 1.0f, TEX_GREENLIGHT},
      {16.5f, 8.5f, 1.0f, TEX_GREENLIGHT}, {16.5f, 16.5f, 1.0f, TEX_GREENLIGHT},
      {6.5f, 5.5f, 1.0f, TEX_PILLAR},      {6.5f, 6.5f, 1.0f, TEX_PILLAR},
      {17.5f, 5.5f, 1.0f, TEX_PILLAR},     {17.5f, 6.5f, 1.0f, TEX_PILLAR},
      {9.5f, 9.5f, 1.0f, TEX_PILLAR},      {14.5f, 9.5f, 1.0f, TEX_PILLAR},
      {4.5f, 18.5f, 1.0f, TEX_BARREL},     {6.5f, 18.5f, 1.0f, TEX_BARREL},
      {9.5f, 18.5f, 1.0f, TEX_BARREL},     {14.5f, 16.5f, 1.0f, TEX_BARREL},
      {17.5f, 16.5f, 1.0f, TEX_BARREL},    {12.5f, 20.5f, 1.0f, TEX_BARREL},
  };

  static const struct
  {
    f32 x, y, scale;
    i32 textureId;
  } pickups[] = {
      {12.5f, 7.5f, 0.9f, TEX_MONEY},
      {11.5f, 15.5f, 0.9f, TEX_MONEY},
  };

  static const struct
  {
    f32 x, y, scale;
    i32 health;
    const char *animation;
  } enemies[] = {
      {15.5f, 13.5f, 1.35f, 100, "DEMON_WALK"},
  };

  for (i32 i = 0; i < (i32)(sizeof(decorations) / sizeof(decorations[0])); ++i)
  {
    const f32 scale = decorations[i].scale;
    Sprite sprite =
        sprite_make(decorations[i].x, decorations[i].y, SPRITE_DECORATION,
                    spriteAppearanceFromTexture(decorations[i].textureId),
                    scale, 0);
    entities_pushSprite(sprite);
  }

  for (i32 i = 0; i < (i32)(sizeof(pickups) / sizeof(pickups[0])); ++i)
  {
    Sprite sprite =
        sprite_make(pickups[i].x, pickups[i].y, SPRITE_PICKUP,
                    spriteAppearanceFromTexture(pickups[i].textureId),
                    pickups[i].scale, 0);
    entities_pushSprite(sprite);
  }

  for (i32 i = 0; i < (i32)(sizeof(enemies) / sizeof(enemies[0])); ++i)
  {
    Animation *anim = animation_from_name(enemies[i].animation);
    Sprite sprite =
        sprite_make(enemies[i].x, enemies[i].y, SPRITE_ENEMY,
                    anim ? spriteAppearanceFromAnimation(anim)
                         : spriteAppearanceFromTexture(TEX_GREENLIGHT),
                    enemies[i].scale, enemies[i].health);
    entities_pushSprite(sprite);
  }
}

static const char *find_matching(const char *start, char open, char close)
{
  int depth = 0;
  const char *p = start;

  while (p && *p)
  {
    if (*p == '\"')
    {
      p++;
      while (*p && *p != '\"')
      {
        if (*p == '\\' && p[1])
          p += 2;
        else
          p++;
      }
    }
    else if (*p == open)
    {
      depth++;
    }
    else if (*p == close)
    {
      depth--;
      if (depth == 0)
        return p;
    }

    if (*p == '\0')
      break;
    p++;
  }

  return NULL;
}

static int json_get_string(const char *start, const char *end,
                           const char *key, char *out, size_t outSize)
{
  char pattern[64];
  snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  size_t patternLen = strlen(pattern);

  const char *pos = start;
  while (pos && pos < end)
  {
    pos = strstr(pos, pattern);
    if (!pos || pos >= end)
      return 0;

    const char *colon = strchr(pos + patternLen, ':');
    if (!colon || colon >= end)
      return -1;

    colon++;
    while (colon < end && isspace((unsigned char)*colon))
      colon++;

    if (colon >= end || *colon != '\"')
      return -1;

    colon++;
    size_t i = 0;
    while (colon < end && *colon != '\"')
    {
      if (i + 1 < outSize)
        out[i++] = *colon;
      colon++;
    }

    if (colon >= end || *colon != '\"')
      return -1;

    out[i] = '\0';
    return 1;
  }

  return 0;
}

static int json_get_double(const char *start, const char *end,
                           const char *key, double *out)
{
  char pattern[64];
  snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  size_t patternLen = strlen(pattern);

  const char *pos = start;
  while (pos && pos < end)
  {
    pos = strstr(pos, pattern);
    if (!pos || pos >= end)
      return 0;

    const char *colon = strchr(pos + patternLen, ':');
    if (!colon || colon >= end)
      return -1;

    colon++;
    while (colon < end && isspace((unsigned char)*colon))
      colon++;

    char *tmp = NULL;
    double value = strtod(colon, &tmp);
    if (colon == tmp)
      return -1;

    *out = value;
    return 1;
  }

  return 0;
}

static void entities_parse_settings(const char *json)
{
  if (!json)
    return;

  char pattern[64];
  snprintf(pattern, sizeof(pattern), "\"%s\"", "settings");
  const char *section = strstr(json, pattern);
  if (!section)
    return;

  const char *objectStart = strchr(section, '{');
  if (!objectStart)
    return;
  const char *objectEnd = find_matching(objectStart, '{', '}');
  if (!objectEnd)
    return;

  double value;
  if (json_get_double(objectStart, objectEnd, "floor_texture", &value) == 1)
    g_floorTextureId = (int)value;
  if (json_get_double(objectStart, objectEnd, "ceiling_texture", &value) == 1)
    g_ceilingTextureId = (int)value;
  if (json_get_double(objectStart, objectEnd, "player_spawn_x", &value) == 1)
    g_playerSpawnX = value;
  if (json_get_double(objectStart, objectEnd, "player_spawn_y", &value) == 1)
    g_playerSpawnY = value;
  if (json_get_double(objectStart, objectEnd, "player_spawn_dir", &value) == 1)
  {
    double wrapped = fmod(value, 360.0);
    if (wrapped < 0.0)
      wrapped += 360.0;
    g_playerSpawnDirDegrees = wrapped;
  }
}

static int texture_from_name(const char *name, i32 *out)
{
  if (strcmp(name, "GREENLIGHT") == 0)
  {
    *out = TEX_GREENLIGHT;
    return 1;
  }
  if (strcmp(name, "PILLAR") == 0)
  {
    *out = TEX_PILLAR;
    return 1;
  }
  if (strcmp(name, "BARREL") == 0)
  {
    *out = TEX_BARREL;
    return 1;
  }
  if (strcmp(name, "MONEY") == 0)
  {
    *out = TEX_MONEY;
    return 1;
  }

  fprintf(stderr, "\033[31m[ERROR] Unknown texture name '%s' in entities.json\033[0m\n",
          name);
  return 0;
}

static Animation *animation_from_name(const char *name)
{
  if (strcmp(name, "DEMON_WALK") == 0)
    return &animations.demon_walk;

  fprintf(stderr,
          "\033[31m[ERROR] Unknown animation name '%s' in entities.json\033[0m\n",
          name);
  return NULL;
}

static int parse_decoration_object(const char *start, const char *end)
{
  double x = 0.0;
  double y = 0.0;
  double scale = 1.0;
  char textureName[32];

  if (json_get_double(start, end, "x", &x) != 1 ||
      json_get_double(start, end, "y", &y) != 1)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Decoration must contain numeric 'x' and 'y'\033[0m\n");
    return -1;
  }

  int scaleResult = json_get_double(start, end, "scale", &scale);
  if (scaleResult == -1)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Invalid 'scale' value for decoration\033[0m\n");
    return -1;
  }
  if (scaleResult == 0)
    scale = 1.0;

  if (json_get_string(start, end, "texture", textureName, sizeof(textureName)) != 1)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Decoration must contain string 'texture'\033[0m\n");
    return -1;
  }

  i32 textureId = 0;
  if (!texture_from_name(textureName, &textureId))
    return -1;

  Sprite sprite =
      sprite_make(x, y, SPRITE_DECORATION,
                  spriteAppearanceFromTexture(textureId), (f32)scale, 0);
  if (!entities_pushSprite(sprite))
  {
    fprintf(stderr,
            "\033[31m[ERROR] Too many sprites, decoration skipped\033[0m\n");
    return -1;
  }

  return 0;
}

static int parse_pickup_object(const char *start, const char *end)
{
  double x = 0.0;
  double y = 0.0;
  double scale = 1.0;
  char textureName[32];

  if (json_get_double(start, end, "x", &x) != 1 ||
      json_get_double(start, end, "y", &y) != 1)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Pickup must contain numeric 'x' and 'y'\033[0m\n");
    return -1;
  }

  int scaleResult = json_get_double(start, end, "scale", &scale);
  if (scaleResult == -1)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Invalid 'scale' value for pickup\033[0m\n");
    return -1;
  }
  if (scaleResult == 0)
    scale = 0.9f;

  if (json_get_string(start, end, "texture", textureName, sizeof(textureName)) != 1)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Pickup must contain string 'texture'\033[0m\n");
    return -1;
  }

  i32 textureId = 0;
  if (!texture_from_name(textureName, &textureId))
    return -1;

  Sprite sprite = sprite_make(x, y, SPRITE_PICKUP,
                              spriteAppearanceFromTexture(textureId),
                              (f32)scale, 0);
  if (!entities_pushSprite(sprite))
  {
    fprintf(stderr,
            "\033[31m[ERROR] Too many sprites, pickup skipped\033[0m\n");
    return -1;
  }

  return 0;
}

static int parse_enemy_object(const char *start, const char *end)
{
  double x = 0.0;
  double y = 0.0;
  double scale = 1.0;
  double healthValue = 0.0;
  char animationName[64];

  if (json_get_double(start, end, "x", &x) != 1 ||
      json_get_double(start, end, "y", &y) != 1)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Enemy must contain numeric 'x' and 'y'\033[0m\n");
    return -1;
  }

  int scaleResult = json_get_double(start, end, "scale", &scale);
  if (scaleResult == -1)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Invalid 'scale' value for enemy\033[0m\n");
    return -1;
  }
  if (scaleResult == 0)
    scale = 1.0;

  if (json_get_double(start, end, "health", &healthValue) != 1)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Enemy must contain numeric 'health'\033[0m\n");
    return -1;
  }

  if (json_get_string(start, end, "animation", animationName,
                      sizeof(animationName)) != 1)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Enemy must contain string 'animation'\033[0m\n");
    return -1;
  }

  Animation *animation = animation_from_name(animationName);
  if (!animation)
    return -1;

  Sprite sprite =
      sprite_make(x, y, SPRITE_ENEMY, spriteAppearanceFromAnimation(animation),
                  (f32)scale, (i32)healthValue);
  if (!entities_pushSprite(sprite))
  {
    fprintf(stderr, "\033[31m[ERROR] Too many sprites, enemy skipped\033[0m\n");
    return -1;
  }

  return 0;
}

static int parse_decal_object(const char *start, const char *end)
{
  double x = 0.0;
  double y = 0.0;
  double doorX = 0.0;
  double doorY = 0.0;
  char typeName[32];

  if (json_get_double(start, end, "x", &x) != 1 ||
      json_get_double(start, end, "y", &y) != 1)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Decal must contain numeric 'x' and 'y'\033[0m\n");
    return -1;
  }

  if (json_get_string(start, end, "type", typeName, sizeof(typeName)) != 1)
  {
    fprintf(stderr, "\033[31m[ERROR] Decal must contain string 'type'\033[0m\n");
    return -1;
  }

  int typeIndex = decal_definition_index(typeName);
  if (typeIndex < 0)
  {
    fprintf(stderr, "\033[31m[ERROR] Unknown decal type '%s'\033[0m\n", typeName);
    return -1;
  }

  if (json_get_double(start, end, "door_x", &doorX) != 1 ||
      json_get_double(start, end, "door_y", &doorY) != 1)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Decal must contain numeric 'door_x' and 'door_y'\033[0m\n");
    return -1;
  }

  const DecalDefinition *definition = &g_decalDefinitions[typeIndex];
  int offTextureId = getTextureIndexByName(definition->offTextureName);
  int onTextureId = getTextureIndexByName(definition->onTextureName);
  if (offTextureId < 0 || onTextureId < 0)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Decal textures for '%s' not found\033[0m\n",
            typeName);
    return -1;
  }

  lever_ensureCapacity(g_leverCount + 1);
  if (!g_levers)
    return -1;

  int tileX = (int)floor(x);
  int tileY = (int)floor(y);
  if (tileX < 0 || tileX >= MAP_WIDTH || tileY < 0 || tileY >= MAP_HEIGHT)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Decal coordinates (%.2f, %.2f) out of bounds\033[0m\n",
            x, y);
    return -1;
  }

  int doorTileX = (int)doorX;
  int doorTileY = (int)doorY;
  if (doorTileX < 0 || doorTileX >= MAP_WIDTH || doorTileY < 0 ||
      doorTileY >= MAP_HEIGHT)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Decal door coordinates (%d, %d) out of bounds\033[0m\n",
            doorTileX, doorTileY);
    return -1;
  }

  int facingX = 0;
  int facingY = 0;
  char facingName[16];
  int facingIndex = -1;
  if (json_get_string(start, end, "facing", facingName, sizeof(facingName)) == 1)
  {
    facingIndex = lever_parseFacing(facingName, &facingX, &facingY);
    if (facingIndex < 0)
    {
      fprintf(stderr,
              "\033[31m[ERROR] Unknown decal facing '%s'\033[0m\n", facingName);
      return -1;
    }
  }

  double openTileValue = 0.0;
  int hasOpenTile = json_get_double(start, end, "open_tile", &openTileValue) == 1;

  LeverInstance lever;
  memset(&lever, 0, sizeof(lever));
  lever.doorX = doorTileX;
  lever.doorY = doorTileY;
  lever.tileX = tileX;
  lever.tileY = tileY;
  lever.leverX = (float)tileX + 0.5f;
  lever.leverY = (float)tileY + 0.5f;
  lever.offTextureId = offTextureId;
  lever.onTextureId = onTextureId;
  lever.activated = 0;
  lever.normalX = facingX;
  lever.normalY = facingY;
  lever.originalTileValue =
      (doorTileX >= 0 && doorTileX < MAP_WIDTH && doorTileY >= 0 && doorTileY < MAP_HEIGHT)
          ? worldMap[doorTileX][doorTileY]
          : 0;
  if (lever.originalTileValue <= 0)
  {
    lever.doorX = lever.tileX;
    lever.doorY = lever.tileY;
    if (lever.doorX >= 0 && lever.doorX < MAP_WIDTH && lever.doorY >= 0 &&
        lever.doorY < MAP_HEIGHT)
      lever.originalTileValue = worldMap[lever.doorX][lever.doorY];
    else
      lever.originalTileValue = 0;
  }
  lever.openTileValue = hasOpenTile ? (int)openTileValue : 0;

  lever_alignPosition(&lever);

  if (lever.tileX < 0 || lever.tileY < 0 || lever.tileX >= MAP_WIDTH ||
      lever.tileY >= MAP_HEIGHT)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Lever position out of bounds (%d,%d)\033[0m\n",
            lever.tileX, lever.tileY);
    return -1;
  }

  if (worldMap[lever.tileX][lever.tileY] <= 0)
  {
    fprintf(stderr,
            "\033[33m[WARN] Lever placed on non-wall tile (%d,%d)\033[0m\n",
            lever.tileX, lever.tileY);
  }

  int index = g_leverCount;
  g_levers[index] = lever;
  g_leverDoorMap[doorTileX][doorTileY] = index;
  g_leverTileMap[lever.tileX][lever.tileY] = index;
  g_leverCount++;

  return 0;
}

static int parse_wall_text_object(const char *start, const char *end)
{
  double valueX = 0.0;
  double valueY = 0.0;
  double scaleValue = 0.6;
  double fontSizeValue = 28.0;
  double colorR = 255.0;
  double colorG = 255.0;
  double colorB = 255.0;
  double colorA = 255.0;
  char facingName[16];
  char textValue[256];

  if (json_get_double(start, end, "x", &valueX) != 1 ||
      json_get_double(start, end, "y", &valueY) != 1)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Wall text missing numeric 'x'/'y'\033[0m\n");
    return -1;
  }

  if (json_get_string(start, end, "text", textValue, sizeof(textValue)) != 1)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Wall text missing 'text' string\033[0m\n");
    return -1;
  }

  if (json_get_string(start, end, "facing", facingName, sizeof(facingName)) != 1)
  {
    snprintf(facingName, sizeof(facingName), "EAST");
  }

  json_get_double(start, end, "scale", &scaleValue);
  json_get_double(start, end, "font_size", &fontSizeValue);
  json_get_double(start, end, "color_r", &colorR);
  json_get_double(start, end, "color_g", &colorG);
  json_get_double(start, end, "color_b", &colorB);
  json_get_double(start, end, "color_a", &colorA);

  if (scaleValue < 0.1)
    scaleValue = 0.1;
  if (scaleValue > 1.0)
    scaleValue = 1.0;
  int fontSize = (int)(fontSizeValue + 0.5);
  if (fontSize < 8)
    fontSize = 8;
  if (fontSize > 96)
    fontSize = 96;

  int faceX = 0;
  int faceY = 0;
  int facingIndex = lever_parseFacing(facingName, &faceX, &faceY);
  if (facingIndex < 0)
  {
    fprintf(stderr, "\033[31m[ERROR] Unknown wall text facing '%s'\033[0m\n",
            facingName);
    return -1;
  }

  int tileX = (int)floor(valueX);
  int tileY = (int)floor(valueY);
  if (tileX < 0 || tileX >= MAP_WIDTH || tileY < 0 || tileY >= MAP_HEIGHT)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Wall text coordinates (%.2f, %.2f) out of bounds\033[0m\n",
            valueX, valueY);
    return -1;
  }

  if (worldMap[tileX][tileY] <= 0)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Wall text requires solid wall at (%d, %d)\033[0m\n",
            tileX, tileY);
    return -1;
  }

  SDL_Color color = {
      (Uint8)clamp_int((int)colorR, 0, 255),
      (Uint8)clamp_int((int)colorG, 0, 255),
      (Uint8)clamp_int((int)colorB, 0, 255),
      (Uint8)clamp_int((int)colorA, 0, 255),
  };

  float coverageX = 1.0f;
  float coverageY = 1.0f;
  u32 *pixels = walltext_render_texture(textValue, fontSize, color, &coverageX,
                                        &coverageY);
  if (!pixels)
    return -1;

  walltext_ensureCapacity(g_wallTextCount + 1);
  if (!g_wallTexts)
  {
    free(pixels);
    return -1;
  }

  WallTextInstance instance;
  instance.tileX = tileX;
  instance.tileY = tileY;
  instance.faceX = faceX;
  instance.faceY = faceY;
  instance.scale = (float)scaleValue;
  instance.pixels = pixels;
  instance.coverageX = coverageX;
  instance.coverageY = coverageY;
  g_wallTexts[g_wallTextCount++] = instance;
  return 0;
}

void entities_tryInteract(Engine *engine)
{
  if (!engine)
    return;

  const double interactRangeSq = 1.4 * 1.4;
  double px = engine->player.posX;
  double py = engine->player.posY;
  double dirX = engine->player.dirX;
  double dirY = engine->player.dirY;

  for (int i = 0; i < g_leverCount; ++i)
  {
    LeverInstance *lever = &g_levers[i];
    double dx = lever->interactX - px;
    double dy = lever->interactY - py;
    double distSq = dx * dx + dy * dy;
    if (distSq > interactRangeSq)
      continue;

    double forward = dx * dirX + dy * dirY;
    if (forward <= 0.0)
      continue;

    lever->activated = !lever->activated;
    if (lever->doorX >= 0 && lever->doorX < MAP_WIDTH &&
        lever->doorY >= 0 && lever->doorY < MAP_HEIGHT)
    {
      if (!lever->activated)
      {
        double doorCenterX = (double)lever->doorX + 0.5;
        double doorCenterY = (double)lever->doorY + 0.5;
        double playerDx = engine->player.posX - doorCenterX;
        double playerDy = engine->player.posY - doorCenterY;
        const double clearance = 0.42;
        bool playerInsideTile =
            ((int)engine->player.posX == lever->doorX &&
             (int)engine->player.posY == lever->doorY) ||
            (fabs(playerDx) < clearance && fabs(playerDy) < clearance);
        if (playerInsideTile)
        {
          lever->activated = 1;
          worldMap[lever->doorX][lever->doorY] = lever->openTileValue;
          continue;
        }
      }

      int newValue =
          lever->activated ? lever->openTileValue : lever->originalTileValue;
      worldMap[lever->doorX][lever->doorY] = newValue;
    }
  }
}

int entities_getLeverTextureAtFace(int tileX, int tileY, int faceX, int faceY,
                                   int *outActivated)
{
  if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT)
    return -1;
  int index = g_leverTileMap[tileX][tileY];
  if (index < 0 || index >= g_leverCount)
    return -1;
  const LeverInstance *lever = &g_levers[index];
  if ((lever->normalX != 0 || lever->normalY != 0) &&
      (lever->normalX != faceX || lever->normalY != faceY))
    return -1;
  if (outActivated)
    *outActivated = lever->activated;
  return lever->activated ? lever->onTextureId : lever->offTextureId;
}

int entities_getWallTextAt(int tileX, int tileY, int faceX, int faceY,
                           const u32 **outPixels, float *outScale,
                           float *outCoverageX, float *outCoverageY)
{
  if (tileX < 0 || tileY < 0 || tileX >= MAP_WIDTH || tileY >= MAP_HEIGHT)
    return 0;

  for (int i = 0; i < g_wallTextCount; ++i)
  {
    const WallTextInstance *text = &g_wallTexts[i];
    if (text->tileX != tileX || text->tileY != tileY)
      continue;
    if ((text->faceX != 0 || text->faceY != 0) &&
        (text->faceX != faceX || text->faceY != faceY))
      continue;
    if (!text->pixels)
      continue;

    if (outPixels)
      *outPixels = text->pixels;
    if (outScale)
      *outScale = text->scale;
    if (outCoverageX)
      *outCoverageX = text->coverageX;
    if (outCoverageY)
      *outCoverageY = text->coverageY;
    return 1;
  }

  return 0;
}

static int parse_entity_array(const char *json, const char *sectionName,
                              int (*parser)(const char *, const char *))
{
  char key[64];
  snprintf(key, sizeof(key), "\"%s\"", sectionName);

  const char *section = strstr(json, key);
  if (!section)
    return 0;

  const char *arrayStart = strchr(section, '[');
  if (!arrayStart)
    return -1;

  const char *arrayEnd = find_matching(arrayStart, '[', ']');
  if (!arrayEnd)
    return -1;

  const char *cursor = arrayStart + 1;
  while (cursor < arrayEnd)
  {
    const char *objStart = strchr(cursor, '{');
    if (!objStart || objStart >= arrayEnd)
      break;

    const char *objEnd = find_matching(objStart, '{', '}');
    if (!objEnd || objEnd > arrayEnd)
      return -1;

    if (parser(objStart, objEnd) != 0)
      return -1;

    cursor = objEnd + 1;
  }

  return 0;
}

static int entities_loadFromJSONFile(const char *path)
{
  FILE *file = fopen(path, "rb");
  if (!file)
  {
    fprintf(stderr, "\033[31m[ERROR] Failed to open entities file '%s'\033[0m\n",
            path);
    return -1;
  }

  if (fseek(file, 0, SEEK_END) != 0)
  {
    fclose(file);
    return -1;
  }
  long length = ftell(file);
  if (length < 0)
  {
    fclose(file);
    return -1;
  }
  rewind(file);

  char *buffer = malloc((size_t)length + 1);
  if (!buffer)
  {
    fclose(file);
    return -1;
  }

  size_t read = fread(buffer, 1, (size_t)length, file);
  fclose(file);
  if (read != (size_t)length)
  {
    free(buffer);
    return -1;
  }
  buffer[length] = '\0';

  entities_resetSpawnToDefaults();
  entities_parse_settings(buffer);

  worldSpriteCount = 0;
  int result = 0;
  lever_clear();
  walltext_clear();

  if (parse_entity_array(buffer, "decorations", parse_decoration_object) != 0)
    result = -1;

  if (result == 0 &&
      parse_entity_array(buffer, "pickups", parse_pickup_object) != 0)
    result = -1;

  if (result == 0 &&
      parse_entity_array(buffer, "enemies", parse_enemy_object) != 0)
    result = -1;

  if (result == 0 &&
      parse_entity_array(buffer, "decals", parse_decal_object) != 0)
    result = -1;

  if (result == 0 &&
      parse_entity_array(buffer, "wall_texts", parse_wall_text_object) != 0)
    result = -1;

  free(buffer);

  if (result != 0)
  {
    worldSpriteCount = 0;
    return -1;
  }

  return 0;
}

void entities_getPlayerSpawn(double *outX, double *outY, double *outDirDegrees)
{
  if (outX)
    *outX = g_playerSpawnX;
  if (outY)
    *outY = g_playerSpawnY;
  if (outDirDegrees)
    *outDirDegrees = g_playerSpawnDirDegrees;
}

Sprite *entities_createWorldSprites(void)
{
  if (worldInitialized)
    return worldSprites;

  worldSpriteCount = 0;
  entities_resetSpawnToDefaults();

  if (entities_loadFromJSONFile("levels/1/entities.json") != 0)
  {
    fprintf(stderr,
            "\033[33m[WARN] Falling back to built-in entities layout\033[0m\n");
    entities_populateDefaults();
  }

  fill_unused_slots();
  worldInitialized = 1;
  return worldSprites;
}

Sprite *entities_getSprites(void)
{
  return worldSprites;
}

i32 entities_getSpriteCount(void)
{
  return worldSpriteCount;
}

void entities_reset(void)
{
  worldInitialized = 0;
  worldSpriteCount = 0;
  lever_clear();
  walltext_clear();
  entities_resetSpawnToDefaults();
}
