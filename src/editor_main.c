#include "cimgui.h"
#include "editor_imgui_backend.h"

#include "raycast.h"
#include "texture.h"
#include "sprites.h"
#define STBI_rgb_alpha 4
#include "third_party/stb_image.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengl.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef IM_COL32
#define EDITOR_COL32(r, g, b, a)                                 \
  (((ImU32)(a) << 24) | ((ImU32)(b) << 16) | ((ImU32)(g) << 8) | \
   (ImU32)(r))
#else
#define EDITOR_COL32 IM_COL32
#endif

typedef struct TileTexture
{
  GLuint id;
  int width;
  int height;
  const char *name;
  ImTextureRef texRef;
  int mapId;
} TileTexture;

static TileTexture *g_tileTextures = NULL;
static int g_tileTextureCount = 0;

typedef enum EditorMode
{
  EDIT_MODE_WALLS = 0,
  EDIT_MODE_DECORATIONS,
  EDIT_MODE_PICKUPS,
  EDIT_MODE_ENEMIES,
  EDIT_MODE_DECALS,
  EDIT_MODE_TEXT,
  EDIT_MODE_SPAWN
} EditorMode;

#ifdef __cplusplus
extern "C"
{
#endif
#include "map.h"
#ifdef __cplusplus
}
#endif

static int g_levelTiles[MAP_WIDTH][MAP_HEIGHT];
static bool g_levelDirty = true;
static bool g_mapNeedsSave = false;
typedef struct LevelInfo
{
  const char *label;
  const char *mapPath;
  const char *entitiesPath;
} LevelInfo;

static const LevelInfo g_levelInfos[] = {
    {"Level 1", "levels/1/map.csv", "levels/1/entities.json"},
    {"Level 2", "levels/2/map.csv", "levels/2/entities.json"},
    {"Level 3", "levels/3/map.csv", "levels/3/entities.json"},
    {"Level 4", "levels/4/map.csv", "levels/4/entities.json"},
    {"Level 5", "levels/5/map.csv", "levels/5/entities.json"},
};
static const int g_levelInfoCount =
    (int)(sizeof(g_levelInfos) / sizeof(g_levelInfos[0]));
static int g_selectedLevelIndex = 0;
static const char *g_mapFilePath = NULL;
static const char *g_entitiesFilePath = NULL;
static void editor_loadMapIntoTiles(void);
static int editor_loadEntities(void);
static int g_floorTextureSelection = 3;
static int g_ceilingTextureSelection = 6;
static int g_selectedWallTileId = 1;

typedef struct DecorationType
{
  const char *label;
  const char *jsonName;
  const char *textureName;
  int textureId;
  float defaultScale;
} DecorationType;

typedef struct PickupType
{
  const char *label;
  const char *jsonName;
  const char *textureName;
  int textureId;
  float defaultScale;
} PickupType;

typedef struct DecalType
{
  const char *label;
  const char *jsonName;
  const char *textureOffName;
  const char *textureOnName;
} DecalType;

static const DecorationType g_decorationTypes[] = {
    {"Green Light", "GREENLIGHT", "GreenLight", TEX_GREENLIGHT, 1.0f},
    {"Pillar", "PILLAR", "Pillar", TEX_PILLAR, 1.0f},
    {"Barrel", "BARREL", "Barrel", TEX_BARREL, 1.0f},
};
static const int g_decorationTypeCount =
    (int)(sizeof(g_decorationTypes) / sizeof(g_decorationTypes[0]));

static const PickupType g_pickupTypes[] = {
    {"Money", "MONEY", "Money", TEX_MONEY, 0.9f},
};
static const int g_pickupTypeCount =
    (int)(sizeof(g_pickupTypes) / sizeof(g_pickupTypes[0]));

static const DecalType g_decalTypes[] = {
    {"Lever", "LEVER", "LeverOff", "LeverOn"},
};
static const int g_decalTypeCount =
    (int)(sizeof(g_decalTypes) / sizeof(g_decalTypes[0]));

static const char *g_decalFacingLabels[] = {"East", "West", "South", "North"};
static const char *g_decalFacingJson[] = {"EAST", "WEST", "SOUTH", "NORTH"};
static const int g_decalFacingVectors[][2] = {
    {1, 0},
    {-1, 0},
    {0, 1},
    {0, -1},
};
static const int g_decalFacingCount =
    (int)(sizeof(g_decalFacingLabels) / sizeof(g_decalFacingLabels[0]));

typedef struct EditorDecoration
{
  float x, y;
  float scale;
  int typeIndex;
} EditorDecoration;

typedef struct EditorPickup
{
  float x, y;
  float scale;
  int typeIndex;
} EditorPickup;

typedef struct EditorDecal
{
  float x, y;
  int typeIndex;
  int doorX;
  int doorY;
  int activated;
  int facingIndex;
  int tileX;
  int tileY;
} EditorDecal;

#define WALL_TEXT_MAX_CHARS 128

typedef struct EditorWallText
{
  float x, y;
  int tileX;
  int tileY;
  int facingIndex;
  char text[WALL_TEXT_MAX_CHARS];
  float scale;
  int fontSize;
  ImVec4 color;
} EditorWallText;

static int editor_findDecorationTypeByJson(const char *jsonName)
{
  if (!jsonName)
    return -1;
  for (int i = 0; i < g_decorationTypeCount; ++i)
    if (strcmp(g_decorationTypes[i].jsonName, jsonName) == 0)
      return i;
  return -1;
}

static int editor_findPickupTypeByJson(const char *jsonName)
{
  if (!jsonName)
    return -1;
  for (int i = 0; i < g_pickupTypeCount; ++i)
    if (strcmp(g_pickupTypes[i].jsonName, jsonName) == 0)
      return i;
  return -1;
}

static int editor_findDecalTypeByJson(const char *jsonName)
{
  if (!jsonName)
    return -1;
  for (int i = 0; i < g_decalTypeCount; ++i)
    if (strcmp(g_decalTypes[i].jsonName, jsonName) == 0)
      return i;
  return -1;
}

typedef struct EditorEnemy
{
  float x, y;
  float scale;
  int health;
  char animation[32];
} EditorEnemy;

typedef struct EnemyType
{
  const char *label;
  const char *animation;
  const char *previewPath;
  int defaultHealth;
  float defaultScale;
} EnemyType;

static const EnemyType g_enemyTypes[] = {
    {"Demon", "DEMON_WALK", "assets/textures/entities/demon/preview/preview.png", 100, 1.35f},
};
static const int g_enemyTypeCount =
    (int)(sizeof(g_enemyTypes) / sizeof(g_enemyTypes[0]));

static EditorEnemy *g_enemies = NULL;
static int g_enemyCount = 0;
static int g_enemyCapacity = 0;
static int g_selectedEnemyIndex = -1;
static int g_selectedEnemyType = 0;
static EditorMode g_editorMode = EDIT_MODE_WALLS;
static int g_selectedDecorationType = 0;
static int g_selectedPickupType = 0;
static int g_selectedDecorationIndex = -1;
static int g_selectedPickupIndex = -1;
static int g_selectedDecalType = 0;
static int g_selectedDecalIndex = -1;
static EditorWallText *g_wallTexts = NULL;
static int g_wallTextCount = 0;
static int g_wallTextCapacity = 0;
static int g_selectedWallTextIndex = -1;
static v2f g_playerSpawnPos = {1.5f, 1.5f};
static f32 g_playerSpawnDirDegrees = 90.0f;
static const v2f g_defaultPlayerSpawnPos = {1.5f, 1.5f};
static const f32 g_defaultPlayerSpawnDirDegrees = 90.0f;
static int g_pendingDoorAssignmentIndex = -1;

static EditorDecoration *g_decorations = NULL;
static int g_decorationCount = 0;
static int g_decorationCapacity = 0;

static EditorPickup *g_pickups = NULL;
static int g_pickupCount = 0;
static int g_pickupCapacity = 0;

static EditorDecal *g_decals = NULL;
static int g_decalCount = 0;
static int g_decalCapacity = 0;

typedef struct EnemyPreviewTexture
{
  GLuint id;
  ImTextureRef texRef;
  int width;
  int height;
} EnemyPreviewTexture;

static EnemyPreviewTexture g_enemyPreviews[sizeof(g_enemyTypes) / sizeof(g_enemyTypes[0])];

static bool g_entitiesDirty = false;
static char g_statusMessage[256] = {0};
static double g_statusMessageExpire = 0.0;
static ImVec4 g_statusColor = {0.3f, 0.8f, 0.4f, 1.0f};

static float editor_clampf(float value, float minValue, float maxValue)
{
  if (value < minValue)
    return minValue;
  if (value > maxValue)
    return maxValue;
  return value;
}

static int editor_clampi(int value, int minValue, int maxValue)
{
  if (value < minValue)
    return minValue;
  if (value > maxValue)
    return maxValue;
  return value;
}

static float editor_wrapDegrees(float degrees)
{
  float wrapped = fmodf(degrees, 360.0f);
  if (wrapped < 0.0f)
    wrapped += 360.0f;
  return wrapped;
}

static void editor_decalFacingVector(int facingIndex, int *outX, int *outY)
{
  if (!outX || !outY)
    return;
  if (facingIndex < 0 || facingIndex >= g_decalFacingCount)
  {
    *outX = 0;
    *outY = 0;
    return;
  }
  *outX = g_decalFacingVectors[facingIndex][0];
  *outY = g_decalFacingVectors[facingIndex][1];
}

static int editor_decalFacingIndexFromVector(int x, int y)
{
  for (int i = 0; i < g_decalFacingCount; ++i)
  {
    if (g_decalFacingVectors[i][0] == x && g_decalFacingVectors[i][1] == y)
      return i;
  }
  return 0;
}

static int editor_equalsIgnoreCase(const char *a, const char *b)
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

static int editor_decalFacingIndexFromString(const char *value)
{
  if (!value)
    return -1;

  for (int i = 0; i < g_decalFacingCount; ++i)
  {
    if (editor_equalsIgnoreCase(value, g_decalFacingJson[i]))
      return i;
    char abbrev[2] = {g_decalFacingJson[i][0], '\0'};
    if (editor_equalsIgnoreCase(value, abbrev))
      return i;
  }
  return -1;
}

static void editor_setSpawnPosition(float x, float y)
{
  float clampedX = editor_clampf(x, 0.5f, (float)MAP_WIDTH - 0.5f);
  float clampedY = editor_clampf(y, 0.5f, (float)MAP_HEIGHT - 0.5f);
  if (fabsf(g_playerSpawnPos.x - clampedX) > 0.0001f ||
      fabsf(g_playerSpawnPos.y - clampedY) > 0.0001f)
  {
    g_playerSpawnPos.x = clampedX;
    g_playerSpawnPos.y = clampedY;
    g_entitiesDirty = true;
  }
}

static void editor_setSpawnDirection(float degrees)
{
  float wrapped = editor_wrapDegrees(degrees);
  if (fabsf(g_playerSpawnDirDegrees - wrapped) > 0.0001f)
  {
    g_playerSpawnDirDegrees = wrapped;
    g_entitiesDirty = true;
  }
}

static ImTextureRef make_texture_ref(GLuint id)
{
  ImTextureRef ref;
  ref._TexData = NULL;
  ref._TexID = (ImTextureID)(uintptr_t)id;
  return ref;
}

static GLuint upload_texture_rgba(const unsigned char *pixels, int width,
                                  int height)
{
  GLuint tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, pixels);
  glBindTexture(GL_TEXTURE_2D, 0);
  return tex;
}

static TileTexture *editor_findTileByName(const char *name)
{
  if (!name)
    return NULL;
  for (int i = 0; i < g_tileTextureCount; ++i)
  {
    if (g_tileTextures[i].name && strcmp(g_tileTextures[i].name, name) == 0)
      return &g_tileTextures[i];
  }
  return NULL;
}

static void editor_ensureDecorationCapacity(int required)
{
  if (required <= g_decorationCapacity)
    return;
  int newCap = g_decorationCapacity ? g_decorationCapacity * 2 : 16;
  while (newCap < required)
    newCap *= 2;
  EditorDecoration *newData =
      (EditorDecoration *)realloc(g_decorations, newCap * sizeof(EditorDecoration));
  if (!newData)
    return;
  g_decorations = newData;
  g_decorationCapacity = newCap;
}

static void editor_ensurePickupCapacity(int required)
{
  if (required <= g_pickupCapacity)
    return;
  int newCap = g_pickupCapacity ? g_pickupCapacity * 2 : 16;
  while (newCap < required)
    newCap *= 2;
  EditorPickup *newData =
      (EditorPickup *)realloc(g_pickups, newCap * sizeof(EditorPickup));
  if (!newData)
    return;
  g_pickups = newData;
  g_pickupCapacity = newCap;
}

static void editor_ensureDecalCapacity(int required)
{
  if (required <= g_decalCapacity)
    return;
  int newCap = g_decalCapacity ? g_decalCapacity * 2 : 8;
  while (newCap < required)
    newCap *= 2;
  EditorDecal *newData =
      (EditorDecal *)realloc(g_decals, newCap * sizeof(EditorDecal));
  if (!newData)
    return;
  g_decals = newData;
  g_decalCapacity = newCap;
}

static void editor_clearDecorations(void)
{
  g_decorationCount = 0;
  g_selectedDecorationIndex = -1;
}

static void editor_clearPickups(void)
{
  g_pickupCount = 0;
  g_selectedPickupIndex = -1;
}

static void editor_clearDecals(void)
{
  g_decalCount = 0;
  g_selectedDecalIndex = -1;
  g_pendingDoorAssignmentIndex = -1;
}

static void editor_clearEnemyPreviews(void)
{
  for (int i = 0; i < g_enemyTypeCount; ++i)
  {
    if (g_enemyPreviews[i].id)
    {
      glDeleteTextures(1, &g_enemyPreviews[i].id);
      g_enemyPreviews[i].id = 0;
    }
    g_enemyPreviews[i].texRef._TexID = (ImTextureID)0;
    g_enemyPreviews[i].texRef._TexData = NULL;
    g_enemyPreviews[i].width = 0;
    g_enemyPreviews[i].height = 0;
  }
}

static EditorDecoration *editor_pushDecoration(const EditorDecoration *source)
{
  if (!source)
    return NULL;
  editor_ensureDecorationCapacity(g_decorationCount + 1);
  if (!g_decorations)
    return NULL;
  g_decorations[g_decorationCount] = *source;
  return &g_decorations[g_decorationCount++];
}

static EditorPickup *editor_pushPickup(const EditorPickup *source)
{
  if (!source)
    return NULL;
  editor_ensurePickupCapacity(g_pickupCount + 1);
  if (!g_pickups)
    return NULL;
  g_pickups[g_pickupCount] = *source;
  return &g_pickups[g_pickupCount++];
}

static EditorDecal *editor_pushDecal(const EditorDecal *source)
{
  if (!source)
    return NULL;
  editor_ensureDecalCapacity(g_decalCount + 1);
  if (!g_decals)
    return NULL;
  g_decals[g_decalCount] = *source;
  return &g_decals[g_decalCount++];
}

static double editor_now_seconds(void)
{
  return SDL_GetTicks() / 1000.0;
}

static void editor_vsetStatus(const ImVec4 *color, const char *fmt, va_list args)
{
  vsnprintf(g_statusMessage, sizeof(g_statusMessage), fmt, args);
  if (color)
    g_statusColor = *color;
  g_statusMessageExpire = editor_now_seconds() + 4.0;
}

static void editor_setStatus(const char *fmt, ...)
{
  ImVec4 color = {0.3f, 0.8f, 0.4f, 1.0f};
  va_list args;
  va_start(args, fmt);
  editor_vsetStatus(&color, fmt, args);
  va_end(args);
}

static void editor_setWarningStatus(const char *fmt, ...)
{
  ImVec4 color = {0.95f, 0.6f, 0.2f, 1.0f};
  va_list args;
  va_start(args, fmt);
  editor_vsetStatus(&color, fmt, args);
  va_end(args);
}

static void editor_setErrorStatus(const char *fmt, ...)
{
  ImVec4 color = {0.9f, 0.3f, 0.3f, 1.0f};
  va_list args;
  va_start(args, fmt);
  editor_vsetStatus(&color, fmt, args);
  va_end(args);
}

static void editor_writeEscapedJsonString(FILE *file, const char *value)
{
  if (!value)
  {
    fputs("\"\"", file);
    return;
  }

  fputc('"', file);
  for (const unsigned char *ptr = (const unsigned char *)value; *ptr; ++ptr)
  {
    unsigned char c = *ptr;
    switch (c)
    {
    case '\\':
    case '"':
      fputc('\\', file);
      fputc(c, file);
      break;
    case '\n':
      fputs("\\n", file);
      break;
    case '\r':
      fputs("\\r", file);
      break;
    case '\t':
      fputs("\\t", file);
      break;
    default:
      if (c < 32)
        fprintf(file, "\\u%04x", c);
      else
        fputc(c, file);
      break;
    }
  }
  fputc('"', file);
}

static void editor_clearStatus(void)
{
  g_statusMessage[0] = '\0';
  g_statusMessageExpire = 0.0;
}

static bool editor_statusActive(void)
{
  if (g_statusMessage[0] == '\0')
    return false;
  if (editor_now_seconds() > g_statusMessageExpire)
  {
    editor_clearStatus();
    return false;
  }
  return true;
}

static int editor_totalEntityCount(void)
{
  return g_decorationCount + g_pickupCount + g_enemyCount + g_decalCount +
         g_wallTextCount;
}

static bool editor_hasSpriteCapacity(void)
{
  return editor_totalEntityCount() < NUM_SPRITES;
}

static int editor_guessDecalFacing(int tileX, int tileY)
{
  static const int lookupOrder[4][2] = {
      {1, 0},
      {-1, 0},
      {0, 1},
      {0, -1},
  };

  for (int i = 0; i < 4; ++i)
  {
    int dirX = lookupOrder[i][0];
    int dirY = lookupOrder[i][1];
    int nx = tileX + dirX;
    int ny = tileY + dirY;
    if (nx < 0 || ny < 0 || nx >= MAP_WIDTH || ny >= MAP_HEIGHT)
      continue;
    if (g_levelTiles[nx][ny] == 0)
      return editor_decalFacingIndexFromVector(dirX, dirY);
  }
  return 0;
}

static void editor_centerDecal(EditorDecal *decal)
{
  if (!decal)
    return;
  int tileX = (int)floorf(decal->x);
  int tileY = (int)floorf(decal->y);
  if (tileX < 0)
    tileX = 0;
  else if (tileX >= MAP_WIDTH)
    tileX = MAP_WIDTH - 1;
  if (tileY < 0)
    tileY = 0;
  else if (tileY >= MAP_HEIGHT)
    tileY = MAP_HEIGHT - 1;
  decal->tileX = tileX;
  decal->tileY = tileY;
  decal->x = (float)tileX + 0.5f;
  decal->y = (float)tileY + 0.5f;
}

static void editor_applyDecalFacing(EditorDecal *decal, int facingIndex,
                                    bool alignDoorTarget)
{
  if (!decal)
    return;

  if (facingIndex < 0 || facingIndex >= g_decalFacingCount)
    facingIndex = 0;

  decal->facingIndex = facingIndex;
  editor_centerDecal(decal);

  if (alignDoorTarget)
  {
    int dirX = 0;
    int dirY = 0;
    editor_decalFacingVector(facingIndex, &dirX, &dirY);
    if (dirX != 0 || dirY != 0)
    {
      int targetX = decal->tileX;
      int targetY = decal->tileY;
      bool found = false;
      while (1)
      {
        targetX += dirX;
        targetY += dirY;
        if (targetX < 0 || targetX >= MAP_WIDTH || targetY < 0 ||
            targetY >= MAP_HEIGHT)
          break;
        if (g_levelTiles[targetX][targetY] != 0)
        {
          decal->doorX = targetX;
          decal->doorY = targetY;
          found = true;
          break;
        }
      }
      if (!found)
      {
        decal->doorX = decal->tileX;
        decal->doorY = decal->tileY;
      }
    }
  }
}

static EditorDecoration *editor_addDecoration(float x, float y, int typeIndex)
{
  if (typeIndex < 0 || typeIndex >= g_decorationTypeCount)
    return NULL;
  if (!editor_hasSpriteCapacity())
  {
    editor_setWarningStatus("Sprite limit reached (%d).", NUM_SPRITES);
    return NULL;
  }
  EditorDecoration decoration;
  decoration.x = x;
  decoration.y = y;
  decoration.scale = g_decorationTypes[typeIndex].defaultScale;
  decoration.typeIndex = typeIndex;
  EditorDecoration *result = editor_pushDecoration(&decoration);
  if (result)
    g_entitiesDirty = true;
  return result;
}

static EditorPickup *editor_addPickup(float x, float y, int typeIndex)
{
  if (typeIndex < 0 || typeIndex >= g_pickupTypeCount)
    return NULL;
  if (!editor_hasSpriteCapacity())
  {
    editor_setWarningStatus("Sprite limit reached (%d).", NUM_SPRITES);
    return NULL;
  }
  EditorPickup pickup;
  pickup.x = x;
  pickup.y = y;
  pickup.scale = g_pickupTypes[typeIndex].defaultScale;
  pickup.typeIndex = typeIndex;
  EditorPickup *result = editor_pushPickup(&pickup);
  if (result)
    g_entitiesDirty = true;
  return result;
}

static EditorDecal *editor_addDecal(float x, float y, int typeIndex)
{
  if (typeIndex < 0 || typeIndex >= g_decalTypeCount)
    return NULL;
  if (!editor_hasSpriteCapacity())
  {
    editor_setWarningStatus("Sprite limit reached (%d).", NUM_SPRITES);
    return NULL;
  }
  int tileX = (int)floorf(x);
  int tileY = (int)floorf(y);
  if (tileX < 0 || tileX >= MAP_WIDTH || tileY < 0 || tileY >= MAP_HEIGHT)
  {
    editor_setErrorStatus("Decal coordinates out of bounds.");
    return NULL;
  }

  if (g_levelTiles[tileX][tileY] == 0)
  {
    editor_setWarningStatus("Decals must be placed on walls.");
    return NULL;
  }

  EditorDecal decal;
  memset(&decal, 0, sizeof(decal));
  decal.x = (float)tileX + 0.5f;
  decal.y = (float)tileY + 0.5f;
  decal.tileX = tileX;
  decal.tileY = tileY;
  decal.typeIndex = typeIndex;
  decal.doorX = tileX;
  decal.doorY = tileY;
  decal.activated = 0;
  int defaultFacing = editor_guessDecalFacing(tileX, tileY);
  editor_applyDecalFacing(&decal, defaultFacing, false);

  EditorDecal *result = editor_pushDecal(&decal);
  if (result)
  {
    g_entitiesDirty = true;
    g_pendingDoorAssignmentIndex = (int)(result - g_decals);
    editor_setStatus("Select door tile for lever.");
  }
  return result;
}

static int editor_findDecorationAtTile(int tileX, int tileY)
{
  for (int i = 0; i < g_decorationCount; ++i)
  {
    int dx = (int)floorf(g_decorations[i].x);
    int dy = (int)floorf(g_decorations[i].y);
    if (dx == tileX && dy == tileY)
      return i;
  }
  return -1;
}

static int editor_findPickupAtTile(int tileX, int tileY)
{
  for (int i = 0; i < g_pickupCount; ++i)
  {
    int px = (int)floorf(g_pickups[i].x);
    int py = (int)floorf(g_pickups[i].y);
    if (px == tileX && py == tileY)
      return i;
  }
  return -1;
}

static int editor_findDecalAtTile(int tileX, int tileY)
{
  for (int i = 0; i < g_decalCount; ++i)
  {
    if (g_decals[i].tileX == tileX && g_decals[i].tileY == tileY)
      return i;
  }
  return -1;
}

static void editor_removeDecorationAtIndex(int index)
{
  if (index < 0 || index >= g_decorationCount)
    return;
  if (index < g_decorationCount - 1)
    memmove(&g_decorations[index], &g_decorations[index + 1],
            (size_t)(g_decorationCount - index - 1) * sizeof(EditorDecoration));
  g_decorationCount--;
  if (g_selectedDecorationIndex >= g_decorationCount)
    g_selectedDecorationIndex = g_decorationCount - 1;
  g_entitiesDirty = true;
}

static void editor_removePickupAtIndex(int index)
{
  if (index < 0 || index >= g_pickupCount)
    return;
  if (index < g_pickupCount - 1)
    memmove(&g_pickups[index], &g_pickups[index + 1],
            (size_t)(g_pickupCount - index - 1) * sizeof(EditorPickup));
  g_pickupCount--;
  if (g_selectedPickupIndex >= g_pickupCount)
    g_selectedPickupIndex = g_pickupCount - 1;
  g_entitiesDirty = true;
}

static void editor_removeDecalAtIndex(int index)
{
  if (index < 0 || index >= g_decalCount)
    return;
  if (g_pendingDoorAssignmentIndex == index)
    g_pendingDoorAssignmentIndex = -1;
  else if (g_pendingDoorAssignmentIndex > index)
    g_pendingDoorAssignmentIndex--;
  if (index < g_decalCount - 1)
    memmove(&g_decals[index], &g_decals[index + 1],
            (size_t)(g_decalCount - index - 1) * sizeof(EditorDecal));
  g_decalCount--;
  if (g_selectedDecalIndex >= g_decalCount)
    g_selectedDecalIndex = g_decalCount - 1;
  g_entitiesDirty = true;
}

static void editor_ensureEnemyCapacity(int required)
{
  if (required <= g_enemyCapacity)
    return;
  int newCap = g_enemyCapacity ? g_enemyCapacity * 2 : 16;
  while (newCap < required)
    newCap *= 2;
  EditorEnemy *newData = (EditorEnemy *)realloc(g_enemies, newCap * sizeof(EditorEnemy));
  if (!newData)
    return;
  g_enemies = newData;
  g_enemyCapacity = newCap;
}

static void editor_clearEnemies(void)
{
  g_enemyCount = 0;
  g_selectedEnemyIndex = -1;
}

static EditorEnemy *editor_pushEnemy(const EditorEnemy *source)
{
  if (!source)
    return NULL;
  editor_ensureEnemyCapacity(g_enemyCount + 1);
  if (!g_enemies)
    return NULL;
  g_enemies[g_enemyCount] = *source;
  return &g_enemies[g_enemyCount++];
}

static EditorEnemy *editor_addEnemy(float x, float y, const EnemyType *type)
{
  if (!editor_hasSpriteCapacity())
  {
    editor_setWarningStatus("Sprite limit reached (%d).", NUM_SPRITES);
    return NULL;
  }
  EditorEnemy enemy;
  enemy.x = x;
  enemy.y = y;
  enemy.scale = type->defaultScale;
  enemy.health = type->defaultHealth;
  snprintf(enemy.animation, sizeof(enemy.animation), "%s", type->animation);
  EditorEnemy *result = editor_pushEnemy(&enemy);
  if (result)
    g_entitiesDirty = true;
  return result;
}

static void editor_ensureWallTextCapacity(int required)
{
  if (required <= g_wallTextCapacity)
    return;
  int newCap = g_wallTextCapacity ? g_wallTextCapacity * 2 : 8;
  while (newCap < required)
    newCap *= 2;
  EditorWallText *newData =
      (EditorWallText *)realloc(g_wallTexts, newCap * sizeof(EditorWallText));
  if (!newData)
    return;
  g_wallTexts = newData;
  g_wallTextCapacity = newCap;
}

static void editor_clearWallTexts(void)
{
  g_wallTextCount = 0;
  g_selectedWallTextIndex = -1;
}

static void editor_centerWallText(EditorWallText *text)
{
  if (!text)
    return;
  int tileX = (int)floorf(text->x);
  int tileY = (int)floorf(text->y);
  if (tileX < 0)
    tileX = 0;
  else if (tileX >= MAP_WIDTH)
    tileX = MAP_WIDTH - 1;
  if (tileY < 0)
    tileY = 0;
  else if (tileY >= MAP_HEIGHT)
    tileY = MAP_HEIGHT - 1;
  text->tileX = tileX;
  text->tileY = tileY;
  text->x = (float)tileX + 0.5f;
  text->y = (float)tileY + 0.5f;
}

static void editor_applyWallTextFacing(EditorWallText *text, int facingIndex)
{
  if (!text)
    return;
  if (facingIndex < 0 || facingIndex >= g_decalFacingCount)
    facingIndex = 0;
  text->facingIndex = facingIndex;
}

static EditorWallText *editor_pushWallText(const EditorWallText *source)
{
  if (!source)
    return NULL;
  editor_ensureWallTextCapacity(g_wallTextCount + 1);
  if (!g_wallTexts)
    return NULL;
  g_wallTexts[g_wallTextCount] = *source;
  return &g_wallTexts[g_wallTextCount++];
}

static EditorWallText *editor_addWallText(float x, float y)
{
  EditorWallText text;
  text.x = x;
  text.y = y;
  text.scale = 0.6f;
  text.fontSize = 28;
  text.color = (ImVec4){1.0f, 1.0f, 1.0f, 1.0f};
  snprintf(text.text, sizeof(text.text), "Click to continue");
  editor_centerWallText(&text);
  int facing = editor_guessDecalFacing(text.tileX, text.tileY);
  editor_applyWallTextFacing(&text, facing);
  EditorWallText *result = editor_pushWallText(&text);
  if (result)
    g_entitiesDirty = true;
  return result;
}

static int editor_findWallTextAtTile(int tileX, int tileY)
{
  for (int i = 0; i < g_wallTextCount; ++i)
  {
    if (g_wallTexts[i].tileX == tileX && g_wallTexts[i].tileY == tileY)
      return i;
  }
  return -1;
}

static void editor_removeWallTextAtIndex(int index)
{
  if (index < 0 || index >= g_wallTextCount)
    return;
  if (index < g_wallTextCount - 1)
    memmove(&g_wallTexts[index], &g_wallTexts[index + 1],
            (size_t)(g_wallTextCount - index - 1) * sizeof(EditorWallText));
  g_wallTextCount--;
  if (g_selectedWallTextIndex >= g_wallTextCount)
    g_selectedWallTextIndex = g_wallTextCount - 1;
  g_entitiesDirty = true;
}

static void editor_removeEnemyAtIndex(int index)
{
  if (index < 0 || index >= g_enemyCount)
    return;
  if (index < g_enemyCount - 1)
    memmove(&g_enemies[index], &g_enemies[index + 1],
            (size_t)(g_enemyCount - index - 1) * sizeof(EditorEnemy));
  g_enemyCount--;
  if (g_selectedEnemyIndex >= g_enemyCount)
    g_selectedEnemyIndex = g_enemyCount - 1;
  g_entitiesDirty = true;
}

static void editor_loadEnemyPreviews(void)
{
  editor_clearEnemyPreviews();

  for (int i = 0; i < g_enemyTypeCount; ++i)
  {
    const EnemyType *type = &g_enemyTypes[i];
    int width = 0;
    int height = 0;
    int comp = 0;
    unsigned char *pixels =
        stbi_load(type->previewPath, &width, &height, &comp, STBI_rgb_alpha);
    if (!pixels || width <= 0 || height <= 0)
    {
      if (pixels)
        stbi_image_free(pixels);
      continue;
    }

    size_t totalPixels = (size_t)width * (size_t)height;
    unsigned char *cursor = pixels;
    for (size_t p = 0; p < totalPixels; ++p)
    {
      if (cursor[0] == 0 && cursor[1] == 255 && cursor[2] == 0)
        cursor[3] = 0;
      cursor += 4;
    }

    GLuint tex = upload_texture_rgba(pixels, width, height);
    stbi_image_free(pixels);

    if (tex)
    {
      g_enemyPreviews[i].id = tex;
      g_enemyPreviews[i].texRef = make_texture_ref(tex);
      g_enemyPreviews[i].width = width;
      g_enemyPreviews[i].height = height;
    }
  }
}

static void editor_applyLevelSelection(int index)
{
  if (index < 0 || index >= g_levelInfoCount)
    index = 0;
  g_selectedLevelIndex = index;
  g_mapFilePath = g_levelInfos[index].mapPath;
  g_entitiesFilePath = g_levelInfos[index].entitiesPath;
}

static const LevelInfo *editor_getCurrentLevel(void)
{
  return &g_levelInfos[g_selectedLevelIndex];
}

static int editor_enemyIndexAtTile(int tileX, int tileY)
{
  for (int i = 0; i < g_enemyCount; ++i)
  {
    int ex = (int)floorf(g_enemies[i].x);
    int ey = (int)floorf(g_enemies[i].y);
    if (ex == tileX && ey == tileY)
      return i;
  }
  return -1;
}

static const char *editor_findMatching(const char *start, char open, char close)
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

static char *editor_extractArray(const char *json, const char *key)
{
  char pattern[64];
  snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  const char *section = strstr(json, pattern);
  if (!section)
    return NULL;

  const char *arrayStart = strchr(section, '[');
  if (!arrayStart)
    return NULL;
  const char *arrayEnd = editor_findMatching(arrayStart, '[', ']');
  if (!arrayEnd)
    return NULL;

  size_t len = (size_t)(arrayEnd - arrayStart + 1);
  char *copy = (char *)malloc(len + 1);
  if (!copy)
    return NULL;
  memcpy(copy, arrayStart, len);
  copy[len] = '\0';
  return copy;
}

static int editor_jsonGetDouble(const char *start, const char *end,
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

static int editor_jsonGetString(const char *start, const char *end,
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

static char *editor_extractObject(const char *json, const char *key)
{
  char pattern[64];
  snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  const char *section = strstr(json, pattern);
  if (!section)
    return NULL;

  const char *objectStart = strchr(section, '{');
  if (!objectStart)
    return NULL;
  const char *objectEnd = editor_findMatching(objectStart, '{', '}');
  if (!objectEnd)
    return NULL;

  size_t len = (size_t)(objectEnd - objectStart + 1);
  char *copy = (char *)malloc(len + 1);
  if (!copy)
    return NULL;
  memcpy(copy, objectStart, len);
  copy[len] = '\0';
  return copy;
}

static char *editor_readFile(const char *path, size_t *outSize)
{
  FILE *file = fopen(path, "rb");
  if (!file)
    return NULL;

  if (fseek(file, 0, SEEK_END) != 0)
  {
    fclose(file);
    return NULL;
  }
  long length = ftell(file);
  if (length < 0)
  {
    fclose(file);
    return NULL;
  }
  rewind(file);

  char *buffer = (char *)malloc((size_t)length + 1);
  if (!buffer)
  {
    fclose(file);
    return NULL;
  }

  size_t read = fread(buffer, 1, (size_t)length, file);
  fclose(file);
  if (read != (size_t)length)
  {
    free(buffer);
    return NULL;
  }

  buffer[length] = '\0';
  if (outSize)
    *outSize = (size_t)length;
  return buffer;
}

static int editor_loadEntities(void)
{
  size_t jsonSize = 0;
  char *json = editor_readFile(g_entitiesFilePath, &jsonSize);
  if (!json)
  {
    fprintf(stderr, "[EDITOR] Failed to read %s\n", g_entitiesFilePath);
    editor_clearDecorations();
    editor_clearPickups();
    editor_clearEnemies();
    editor_clearDecals();
    editor_clearWallTexts();
    g_entitiesDirty = false;
    return -1;
  }

  editor_clearDecorations();
  editor_clearPickups();
  editor_clearEnemies();
  editor_clearDecals();
  editor_clearWallTexts();
  g_playerSpawnPos = g_defaultPlayerSpawnPos;
  g_playerSpawnDirDegrees = g_defaultPlayerSpawnDirDegrees;

  int status = 0;
  bool limitWarned = false;

  char *decorationsArray = editor_extractArray(json, "decorations");
  if (decorationsArray)
  {
    const char *cursor = decorationsArray + 1;
    while (cursor && *cursor)
    {
      const char *objStart = strchr(cursor, '{');
      if (!objStart)
        break;
      const char *objEnd = editor_findMatching(objStart, '{', '}');
      if (!objEnd)
        break;

      double valueX = 0.0;
      double valueY = 0.0;
      double valueScale = 0.0;
      char textureName[32];

      if (editor_jsonGetDouble(objStart, objEnd, "x", &valueX) != 1 ||
          editor_jsonGetDouble(objStart, objEnd, "y", &valueY) != 1)
      {
        fprintf(stderr, "[EDITOR] Decoration entry missing coordinates\n");
        status = -1;
        cursor = objEnd + 1;
        continue;
      }

      int scaleResult = editor_jsonGetDouble(objStart, objEnd, "scale", &valueScale);
      if (scaleResult != 1)
        valueScale = 0.0;

      int texResult =
          editor_jsonGetString(objStart, objEnd, "texture", textureName, sizeof(textureName));
      int typeIndex = editor_findDecorationTypeByJson(texResult == 1 ? textureName : NULL);
      if (typeIndex < 0)
      {
        fprintf(stderr, "[EDITOR] Unknown decoration texture '%s'\n",
                texResult == 1 ? textureName : "(null)");
        typeIndex = 0;
      }

      if (!editor_hasSpriteCapacity())
      {
        if (!limitWarned)
        {
          editor_setWarningStatus("Sprite limit reached (%d); extra entities ignored.",
                                  NUM_SPRITES);
          limitWarned = true;
        }
        cursor = objEnd + 1;
        continue;
      }

      EditorDecoration decoration;
      decoration.x = (float)valueX;
      decoration.y = (float)valueY;
      decoration.typeIndex = typeIndex;
      if (scaleResult == 1)
        decoration.scale = (float)valueScale;
      else
        decoration.scale = g_decorationTypes[typeIndex].defaultScale;

      editor_pushDecoration(&decoration);
      cursor = objEnd + 1;
    }
    free(decorationsArray);
  }

  char *pickupsArray = editor_extractArray(json, "pickups");
  if (pickupsArray)
  {
    const char *cursor = pickupsArray + 1;
    while (cursor && *cursor)
    {
      const char *objStart = strchr(cursor, '{');
      if (!objStart)
        break;
      const char *objEnd = editor_findMatching(objStart, '{', '}');
      if (!objEnd)
        break;

      double valueX = 0.0;
      double valueY = 0.0;
      double valueScale = 0.0;
      char textureName[32];

      if (editor_jsonGetDouble(objStart, objEnd, "x", &valueX) != 1 ||
          editor_jsonGetDouble(objStart, objEnd, "y", &valueY) != 1)
      {
        fprintf(stderr, "[EDITOR] Pickup entry missing coordinates\n");
        status = -1;
        cursor = objEnd + 1;
        continue;
      }

      int scaleResult = editor_jsonGetDouble(objStart, objEnd, "scale", &valueScale);
      if (scaleResult != 1)
        valueScale = 0.0;

      int texResult =
          editor_jsonGetString(objStart, objEnd, "texture", textureName, sizeof(textureName));
      int typeIndex = editor_findPickupTypeByJson(texResult == 1 ? textureName : NULL);
      if (typeIndex < 0)
      {
        fprintf(stderr, "[EDITOR] Unknown pickup texture '%s'\n",
                texResult == 1 ? textureName : "(null)");
        typeIndex = 0;
      }

      if (!editor_hasSpriteCapacity())
      {
        if (!limitWarned)
        {
          editor_setWarningStatus("Sprite limit reached (%d); extra entities ignored.",
                                  NUM_SPRITES);
          limitWarned = true;
        }
        cursor = objEnd + 1;
        continue;
      }

      EditorPickup pickup;
      pickup.x = (float)valueX;
      pickup.y = (float)valueY;
      pickup.typeIndex = typeIndex;
      if (scaleResult == 1)
        pickup.scale = (float)valueScale;
      else
        pickup.scale = g_pickupTypes[typeIndex].defaultScale;

      editor_pushPickup(&pickup);
      cursor = objEnd + 1;
    }
    free(pickupsArray);
  }

  char *decalsArray = editor_extractArray(json, "decals");
  if (decalsArray)
  {
    const char *cursor = decalsArray + 1;
    while (cursor && *cursor)
    {
      const char *objStart = strchr(cursor, '{');
      if (!objStart)
        break;
      const char *objEnd = editor_findMatching(objStart, '{', '}');
      if (!objEnd)
        break;

      double valueX = 0.0;
      double valueY = 0.0;
      double doorX = 0.0;
      double doorY = 0.0;
      char typeName[32];

      if (editor_jsonGetDouble(objStart, objEnd, "x", &valueX) != 1 ||
          editor_jsonGetDouble(objStart, objEnd, "y", &valueY) != 1)
      {
        fprintf(stderr, "[EDITOR] Decal entry missing coordinates\n");
        status = -1;
        cursor = objEnd + 1;
        continue;
      }

      int typeResult =
          editor_jsonGetString(objStart, objEnd, "type", typeName, sizeof(typeName));
      int typeIndex = editor_findDecalTypeByJson(typeResult == 1 ? typeName : NULL);
      if (typeIndex < 0)
      {
        fprintf(stderr, "[EDITOR] Unknown decal type '%s'\n",
                typeResult == 1 ? typeName : "(null)");
        typeIndex = 0;
      }

      if (editor_jsonGetDouble(objStart, objEnd, "door_x", &doorX) != 1 ||
          editor_jsonGetDouble(objStart, objEnd, "door_y", &doorY) != 1)
      {
        fprintf(stderr, "[EDITOR] Decal missing door coordinates\n");
        status = -1;
        cursor = objEnd + 1;
        continue;
      }

      if (!editor_hasSpriteCapacity())
      {
        if (!limitWarned)
        {
          editor_setWarningStatus("Sprite limit reached (%d); extra entities ignored.",
                                  NUM_SPRITES);
          limitWarned = true;
        }
        cursor = objEnd + 1;
        continue;
      }

      char facingName[16];
      int facingIndex =
          editor_jsonGetString(objStart, objEnd, "facing", facingName,
                               sizeof(facingName)) == 1
              ? editor_decalFacingIndexFromString(facingName)
              : -1;

      int tileX = (int)floor(valueX);
      int tileY = (int)floor(valueY);
      if (tileX < 0 || tileX >= MAP_WIDTH || tileY < 0 || tileY >= MAP_HEIGHT)
      {
        fprintf(stderr, "[EDITOR] Decal coordinates out of bounds\n");
        cursor = objEnd + 1;
        continue;
      }

      int doorTileX = (int)doorX;
      int doorTileY = (int)doorY;
      if (doorTileX < 0)
        doorTileX = 0;
      else if (doorTileX >= MAP_WIDTH)
        doorTileX = MAP_WIDTH - 1;
      if (doorTileY < 0)
        doorTileY = 0;
      else if (doorTileY >= MAP_HEIGHT)
        doorTileY = MAP_HEIGHT - 1;

      EditorDecal decal;
      memset(&decal, 0, sizeof(decal));
      decal.x = (float)valueX;
      decal.y = (float)valueY;
      decal.typeIndex = typeIndex;
      decal.doorX = doorTileX;
      decal.doorY = doorTileY;
      decal.activated = 0;
      editor_centerDecal(&decal);
      if (facingIndex < 0)
        facingIndex = editor_guessDecalFacing(decal.tileX, decal.tileY);
      editor_applyDecalFacing(&decal, facingIndex, false);
      if (g_levelTiles[decal.doorX][decal.doorY] == 0)
        editor_applyDecalFacing(&decal, decal.facingIndex, true);

      EditorDecal *slot = editor_pushDecal(&decal);
      if (slot)
        slot->facingIndex = decal.facingIndex;

      cursor = objEnd + 1;
    }
    free(decalsArray);
  }

  char *wallTextsArray = editor_extractArray(json, "wall_texts");
  if (wallTextsArray)
  {
    const char *cursor = wallTextsArray + 1;
    while (cursor && *cursor)
    {
      const char *objStart = strchr(cursor, '{');
      if (!objStart)
        break;
      const char *objEnd = editor_findMatching(objStart, '{', '}');
      if (!objEnd)
        break;

      double posX = 0.0;
      double posY = 0.0;
      double scaleValue = 0.6;
      double fontSizeValue = 28.0;
      double colorR = 255.0;
      double colorG = 255.0;
      double colorB = 255.0;
      double colorA = 255.0;
      char facingName[16];
      char textBuffer[WALL_TEXT_MAX_CHARS];

      if (editor_jsonGetDouble(objStart, objEnd, "x", &posX) != 1 ||
          editor_jsonGetDouble(objStart, objEnd, "y", &posY) != 1)
      {
        fprintf(stderr, "[EDITOR] Wall text missing coordinates\n");
        cursor = objEnd + 1;
        continue;
      }

      if (editor_jsonGetString(objStart, objEnd, "text", textBuffer,
                               sizeof(textBuffer)) != 1)
      {
        fprintf(stderr, "[EDITOR] Wall text missing text content\n");
        cursor = objEnd + 1;
        continue;
      }

      if (editor_jsonGetString(objStart, objEnd, "facing", facingName,
                               sizeof(facingName)) != 1)
      {
        snprintf(facingName, sizeof(facingName), "EAST");
      }

      editor_jsonGetDouble(objStart, objEnd, "scale", &scaleValue);
      editor_jsonGetDouble(objStart, objEnd, "font_size", &fontSizeValue);
      editor_jsonGetDouble(objStart, objEnd, "color_r", &colorR);
      editor_jsonGetDouble(objStart, objEnd, "color_g", &colorG);
      editor_jsonGetDouble(objStart, objEnd, "color_b", &colorB);
      editor_jsonGetDouble(objStart, objEnd, "color_a", &colorA);

      EditorWallText text;
      text.x = (float)posX;
      text.y = (float)posY;
      text.scale = (float)scaleValue;
      if (text.scale < 0.1f)
        text.scale = 0.1f;
      if (text.scale > 1.0f)
        text.scale = 1.0f;
      text.fontSize = (int)(fontSizeValue + 0.5);
      if (text.fontSize < 8)
        text.fontSize = 8;
      if (text.fontSize > 96)
        text.fontSize = 96;
      text.color.x = (float)(colorR / 255.0);
      text.color.y = (float)(colorG / 255.0);
      text.color.z = (float)(colorB / 255.0);
      text.color.w = (float)(colorA / 255.0);
      if (text.color.x < 0.0f)
        text.color.x = 0.0f;
      if (text.color.x > 1.0f)
        text.color.x = 1.0f;
      if (text.color.y < 0.0f)
        text.color.y = 0.0f;
      if (text.color.y > 1.0f)
        text.color.y = 1.0f;
      if (text.color.z < 0.0f)
        text.color.z = 0.0f;
      if (text.color.z > 1.0f)
        text.color.z = 1.0f;
      if (text.color.w < 0.0f)
        text.color.w = 0.0f;
      if (text.color.w > 1.0f)
        text.color.w = 1.0f;
      editor_centerWallText(&text);
      int facingIdx = editor_decalFacingIndexFromString(facingName);
      if (facingIdx < 0)
        facingIdx = 0;
      editor_applyWallTextFacing(&text, facingIdx);
      snprintf(text.text, sizeof(text.text), "%s", textBuffer);
      editor_pushWallText(&text);

      cursor = objEnd + 1;
    }
    free(wallTextsArray);
  }

  char *enemiesArray = editor_extractArray(json, "enemies");
  if (enemiesArray)
  {
    const char *cursor = enemiesArray + 1;
    while (cursor && *cursor)
    {
      const char *objStart = strchr(cursor, '{');
      if (!objStart)
        break;
      const char *objEnd = editor_findMatching(objStart, '{', '}');
      if (!objEnd)
        break;

      if (!editor_hasSpriteCapacity())
      {
        if (!limitWarned)
        {
          editor_setWarningStatus("Sprite limit reached (%d); extra entities ignored.",
                                  NUM_SPRITES);
          limitWarned = true;
        }
        cursor = objEnd + 1;
        continue;
      }

      EditorEnemy enemy;
      enemy.scale = g_enemyTypes[0].defaultScale;
      enemy.health = g_enemyTypes[0].defaultHealth;
      enemy.animation[0] = '\0';

      double value;
      if (editor_jsonGetDouble(objStart, objEnd, "x", &value) == 1)
        enemy.x = (float)value;
      else
        enemy.x = 0.5f;

      if (editor_jsonGetDouble(objStart, objEnd, "y", &value) == 1)
        enemy.y = (float)value;
      else
        enemy.y = 0.5f;

      if (editor_jsonGetDouble(objStart, objEnd, "scale", &value) == 1)
        enemy.scale = (float)value;

      if (editor_jsonGetDouble(objStart, objEnd, "health", &value) == 1)
        enemy.health = (int)value;

      if (editor_jsonGetString(objStart, objEnd, "animation", enemy.animation,
                               sizeof(enemy.animation)) != 1)
      {
        snprintf(enemy.animation, sizeof(enemy.animation), "%s",
                 g_enemyTypes[0].animation);
      }

      editor_pushEnemy(&enemy);
      cursor = objEnd + 1;
    }
    free(enemiesArray);
  }

  char *settings = editor_extractObject(json, "settings");
  if (settings)
  {
    const char *start = settings;
    const char *end = settings + strlen(settings);
    double value;
    if (editor_jsonGetDouble(start, end, "floor_texture", &value) == 1)
    {
      g_floorTextureSelection = (int)value;
      g_floorTextureId = g_floorTextureSelection;
    }
    if (editor_jsonGetDouble(start, end, "ceiling_texture", &value) == 1)
    {
      g_ceilingTextureSelection = (int)value;
      g_ceilingTextureId = g_ceilingTextureSelection;
    }
    if (editor_jsonGetDouble(start, end, "player_spawn_x", &value) == 1)
      g_playerSpawnPos.x =
          editor_clampf((float)value, 0.5f, (float)MAP_WIDTH - 0.5f);
    if (editor_jsonGetDouble(start, end, "player_spawn_y", &value) == 1)
      g_playerSpawnPos.y =
          editor_clampf((float)value, 0.5f, (float)MAP_HEIGHT - 0.5f);
    if (editor_jsonGetDouble(start, end, "player_spawn_dir", &value) == 1)
      g_playerSpawnDirDegrees = editor_wrapDegrees((float)value);
    free(settings);
  }
  else
  {
    g_floorTextureSelection = g_floorTextureId;
    g_ceilingTextureSelection = g_ceilingTextureId;
  }

  if (limitWarned && status == 0)
    status = 1;
  g_entitiesDirty = false;
  free(json);
  return status;
}

static void editor_reloadLevel(bool announceSuccess, const char *successFmt)
{
  const LevelInfo *level = editor_getCurrentLevel();
  int mapResult = map_loadFromCSV(g_mapFilePath);
  editor_loadMapIntoTiles();
  int entityResult = editor_loadEntities();

  g_levelDirty = true;
  g_mapNeedsSave = false;
  g_entitiesDirty = false;

  if (mapResult != 0)
  {
    editor_setErrorStatus("Map load failed for %s; using default layout.", level->label);
    return;
  }
  if (entityResult < 0)
  {
    editor_setErrorStatus("Entity load failed for %s; defaults restored.", level->label);
    return;
  }
  if (entityResult > 0)
    return;

  if (announceSuccess && successFmt)
    editor_setStatus(successFmt, level->label);
}

static void editor_changeLevel(int index)
{
  if (index < 0 || index >= g_levelInfoCount)
    return;
  if (index == g_selectedLevelIndex)
    return;

  editor_applyLevelSelection(index);
  g_selectedDecorationIndex = -1;
  g_selectedPickupIndex = -1;
  g_selectedEnemyIndex = -1;
  g_selectedDecalIndex = -1;

  editor_reloadLevel(true, "Switched to %s.");
}

static void editor_saveEntities(void)
{
  FILE *file = fopen(g_entitiesFilePath, "w");
  if (!file)
  {
    fprintf(stderr, "[EDITOR] Failed to open %s for writing\n",
            g_entitiesFilePath);
    return;
  }

  fprintf(file, "{\n");
  fprintf(file, "  \"settings\": {\n");
  fprintf(file, "    \"floor_texture\": %d,\n", g_floorTextureSelection);
  fprintf(file, "    \"ceiling_texture\": %d,\n", g_ceilingTextureSelection);
  fprintf(file, "    \"player_spawn_x\": %.3f,\n", g_playerSpawnPos.x);
  fprintf(file, "    \"player_spawn_y\": %.3f,\n", g_playerSpawnPos.y);
  fprintf(file, "    \"player_spawn_dir\": %.3f\n", g_playerSpawnDirDegrees);
  fprintf(file, "  },\n");
  fprintf(file, "  \"decorations\": [\n");

  for (int i = 0; i < g_decorationCount; ++i)
  {
    const EditorDecoration *decor = &g_decorations[i];
    const DecorationType *type = &g_decorationTypes[decor->typeIndex];
    fprintf(file, "    {\n");
    fprintf(file, "      \"type\": \"decoration\",\n");
    fprintf(file, "      \"texture\": \"%s\",\n", type->jsonName);
    fprintf(file, "      \"x\": %.3f,\n", decor->x);
    fprintf(file, "      \"y\": %.3f,\n", decor->y);
    fprintf(file, "      \"scale\": %.3f\n", decor->scale);
    fprintf(file, "    }%s\n", (i + 1 < g_decorationCount) ? "," : "");
  }

  fprintf(file, "  ],\n");
  fprintf(file, "  \"pickups\": [\n");

  for (int i = 0; i < g_pickupCount; ++i)
  {
    const EditorPickup *pickup = &g_pickups[i];
    const PickupType *type = &g_pickupTypes[pickup->typeIndex];
    fprintf(file, "    {\n");
    fprintf(file, "      \"type\": \"pickup\",\n");
    fprintf(file, "      \"texture\": \"%s\",\n", type->jsonName);
    fprintf(file, "      \"x\": %.3f,\n", pickup->x);
    fprintf(file, "      \"y\": %.3f,\n", pickup->y);
    fprintf(file, "      \"scale\": %.3f\n", pickup->scale);
    fprintf(file, "    }%s\n", (i + 1 < g_pickupCount) ? "," : "");
  }

  fprintf(file, "  ],\n");
  fprintf(file, "  \"decals\": [\n");

  for (int i = 0; i < g_decalCount; ++i)
  {
    const EditorDecal *decal = &g_decals[i];
    const DecalType *type = &g_decalTypes[decal->typeIndex];
    const char *facingName =
        (decal->facingIndex >= 0 && decal->facingIndex < g_decalFacingCount)
            ? g_decalFacingJson[decal->facingIndex]
            : g_decalFacingJson[0];
    fprintf(file, "    {\n");
    fprintf(file, "      \"type\": \"%s\",\n", type->jsonName);
    fprintf(file, "      \"x\": %.3f,\n", decal->x);
    fprintf(file, "      \"y\": %.3f,\n", decal->y);
    fprintf(file, "      \"facing\": \"%s\",\n", facingName);
    fprintf(file, "      \"door_x\": %d,\n", decal->doorX);
    fprintf(file, "      \"door_y\": %d\n", decal->doorY);
    fprintf(file, "    }%s\n", (i + 1 < g_decalCount) ? "," : "");
  }

  fprintf(file, "  ],\n");
  fprintf(file, "  \"wall_texts\": [\n");

  for (int i = 0; i < g_wallTextCount; ++i)
  {
    const EditorWallText *text = &g_wallTexts[i];
    const char *facingName =
        (text->facingIndex >= 0 && text->facingIndex < g_decalFacingCount)
            ? g_decalFacingJson[text->facingIndex]
            : g_decalFacingJson[0];
    int colorR = editor_clampi((int)(text->color.x * 255.0f + 0.5f), 0, 255);
    int colorG = editor_clampi((int)(text->color.y * 255.0f + 0.5f), 0, 255);
    int colorB = editor_clampi((int)(text->color.z * 255.0f + 0.5f), 0, 255);
    int colorA = editor_clampi((int)(text->color.w * 255.0f + 0.5f), 0, 255);

    fprintf(file, "    {\n");
    fprintf(file, "      \"text\": ");
    editor_writeEscapedJsonString(file, text->text);
    fprintf(file, ",\n");
    fprintf(file, "      \"x\": %.3f,\n", text->x);
    fprintf(file, "      \"y\": %.3f,\n", text->y);
    fprintf(file, "      \"facing\": \"%s\",\n", facingName);
    fprintf(file, "      \"scale\": %.3f,\n", text->scale);
    fprintf(file, "      \"font_size\": %d,\n", text->fontSize);
    fprintf(file, "      \"color_r\": %d,\n", colorR);
    fprintf(file, "      \"color_g\": %d,\n", colorG);
    fprintf(file, "      \"color_b\": %d,\n", colorB);
    fprintf(file, "      \"color_a\": %d\n", colorA);
    fprintf(file, "    }%s\n", (i + 1 < g_wallTextCount) ? "," : "");
  }

  fprintf(file, "  ],\n");
  fprintf(file, "  \"enemies\": [\n");

  for (int i = 0; i < g_enemyCount; ++i)
  {
    const EditorEnemy *enemy = &g_enemies[i];
    fprintf(file, "    {\n");
    fprintf(file, "      \"type\": \"enemy\",\n");
    fprintf(file, "      \"animation\": \"%s\",\n", enemy->animation);
    fprintf(file, "      \"x\": %.3f,\n", enemy->x);
    fprintf(file, "      \"y\": %.3f,\n", enemy->y);
    fprintf(file, "      \"scale\": %.3f,\n", enemy->scale);
    fprintf(file, "      \"health\": %d\n", enemy->health);
    fprintf(file, "    }%s\n", (i + 1 < g_enemyCount) ? "," : "");
  }

  fprintf(file, "  ]\n");
  fprintf(file, "}\n");
  fclose(file);
  printf("[EDITOR] Saved entities to %s\n", g_entitiesFilePath);
  g_entitiesDirty = false;
}

static void editor_loadTileTextures(void)
{
  const int total =
      NUM_WALL_TEXTURES + NUM_DECOR_TEXTURES + NUM_ENTITY_TEXTURES +
      NUM_DECAL_TEXTURES;
  g_tileTextures = (TileTexture *)calloc((size_t)total, sizeof(TileTexture));
  if (!g_tileTextures)
    return;

  g_tileTextureCount = 0;

  for (int i = 0; i < NUM_WALL_TEXTURES; ++i)
  {
    int w, h, c;
    unsigned char *pixels =
        stbi_load(wallTextures[i].path, &w, &h, &c, STBI_rgb_alpha);
    if (!pixels)
      continue;
    TileTexture *slot = &g_tileTextures[g_tileTextureCount++];
    slot->id = upload_texture_rgba(pixels, w, h);
    slot->width = w;
    slot->height = h;
    slot->name = wallTextures[i].name;
    slot->texRef = make_texture_ref(slot->id);
    slot->mapId = i + 1;
    stbi_image_free(pixels);
  }

  for (int i = 0; i < NUM_DECOR_TEXTURES; ++i)
  {
    int w, h, c;
    unsigned char *pixels =
        stbi_load(decorTextures[i].path, &w, &h, &c, STBI_rgb_alpha);
    if (!pixels)
      continue;
    TileTexture *slot = &g_tileTextures[g_tileTextureCount++];
    slot->id = upload_texture_rgba(pixels, w, h);
    slot->width = w;
    slot->height = h;
    slot->name = decorTextures[i].name;
    slot->texRef = make_texture_ref(slot->id);
    slot->mapId = -1;
    stbi_image_free(pixels);
  }

  for (int i = 0; i < NUM_ENTITY_TEXTURES; ++i)
  {
    int w, h, c;
    unsigned char *pixels =
        stbi_load(entityTextures[i].path, &w, &h, &c, STBI_rgb_alpha);
    if (!pixels)
      continue;
    TileTexture *slot = &g_tileTextures[g_tileTextureCount++];
    slot->id = upload_texture_rgba(pixels, w, h);
    slot->width = w;
    slot->height = h;
    slot->name = entityTextures[i].name;
    slot->texRef = make_texture_ref(slot->id);
    slot->mapId = -1;
    stbi_image_free(pixels);
  }

  for (int i = 0; i < NUM_DECAL_TEXTURES; ++i)
  {
    int w, h, c;
    unsigned char *pixels =
        stbi_load(decalTextures[i].path, &w, &h, &c, STBI_rgb_alpha);
    if (!pixels)
      continue;
    TileTexture *slot = &g_tileTextures[g_tileTextureCount++];
    slot->id = upload_texture_rgba(pixels, w, h);
    slot->width = w;
    slot->height = h;
    slot->name = decalTextures[i].name;
    slot->texRef = make_texture_ref(slot->id);
    slot->mapId = -1;
    stbi_image_free(pixels);
  }
}

static void editor_freeTileTextures(void)
{
  if (!g_tileTextures)
    return;

  for (int i = 0; i < g_tileTextureCount; ++i)
  {
    if (g_tileTextures[i].id)
      glDeleteTextures(1, &g_tileTextures[i].id);
  }
  free(g_tileTextures);
  g_tileTextures = NULL;
  g_tileTextureCount = 0;
}

static void editor_loadMapIntoTiles(void)
{
  for (int x = 0; x < MAP_WIDTH; ++x)
  {
    for (int y = 0; y < MAP_HEIGHT; ++y)
      g_levelTiles[x][y] = worldMap[x][y];
  }
  g_levelDirty = true;
  g_mapNeedsSave = false;
}

static void editor_syncTilesToEngine(void)
{
  for (int x = 0; x < MAP_WIDTH; ++x)
  {
    for (int y = 0; y < MAP_HEIGHT; ++y)
      worldMap[x][y] = g_levelTiles[x][y];
  }
}

static void editor_saveMap(void)
{
  FILE *file = fopen(g_mapFilePath, "w");
  if (!file)
  {
    fprintf(stderr, "[EDITOR] Failed to open %s for writing\n", g_mapFilePath);
    return;
  }

  for (int y = 0; y < MAP_HEIGHT; ++y)
  {
    for (int x = 0; x < MAP_WIDTH; ++x)
    {
      fprintf(file, "%d", g_levelTiles[x][y]);
      if (x < MAP_WIDTH - 1)
        fputc(',', file);
    }
    fputc('\n', file);
  }

  fclose(file);
  printf("[EDITOR] Saved map to %s\n", g_mapFilePath);
  g_mapNeedsSave = false;
}

static void editor_fillTiles(int tileId)
{
  for (int x = 0; x < MAP_WIDTH; ++x)
  {
    for (int y = 0; y < MAP_HEIGHT; ++y)
      g_levelTiles[x][y] = tileId;
  }
  g_levelDirty = true;
  g_mapNeedsSave = true;
}

static void editor_drawFloorCeilingControls(void)
{
  if (g_floorTextureSelection < 0 ||
      g_floorTextureSelection >= NUM_WALL_TEXTURES)
  {
    g_floorTextureSelection = 0;
    g_floorTextureId = 0;
  }
  if (g_ceilingTextureSelection < 0 ||
      g_ceilingTextureSelection >= NUM_WALL_TEXTURES)
  {
    g_ceilingTextureSelection = 0;
    g_ceilingTextureId = 0;
  }

  const char *floorPreview = wallTextures[g_floorTextureSelection].name;
  if (igBeginCombo("Floor Texture", floorPreview, 0))
  {
    for (int i = 0; i < NUM_WALL_TEXTURES; ++i)
    {
      bool selected = (i == g_floorTextureSelection);
      if (igSelectable_Bool(wallTextures[i].name, selected, 0,
                            (ImVec2){0.0f, 0.0f}))
      {
        g_floorTextureSelection = i;
        g_floorTextureId = i;
        g_entitiesDirty = true;
      }
      if (selected)
        igSetItemDefaultFocus();
    }
    igEndCombo();
  }

  const char *ceilingPreview = wallTextures[g_ceilingTextureSelection].name;
  if (igBeginCombo("Ceiling Texture", ceilingPreview, 0))
  {
    for (int i = 0; i < NUM_WALL_TEXTURES; ++i)
    {
      bool selected = (i == g_ceilingTextureSelection);
      if (igSelectable_Bool(wallTextures[i].name, selected, 0,
                            (ImVec2){0.0f, 0.0f}))
      {
        g_ceilingTextureSelection = i;
        g_ceilingTextureId = i;
        g_entitiesDirty = true;
      }
      if (selected)
        igSetItemDefaultFocus();
    }
    igEndCombo();
  }
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0)
  {
    fprintf(stderr, "Failed to init SDL: %s\n", SDL_GetError());
    return 1;
  }

  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
  {
    fprintf(stderr, "Failed to init SDL_image: %s\n", IMG_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_Window *window = SDL_CreateWindow(
      "Level Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280,
      720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (!window)
  {
    fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (!gl_context)
  {
    fprintf(stderr, "Failed to create GL context: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_GL_SetSwapInterval(1);

  igCreateContext(NULL);
  ImGuiIO *io = igGetIO_Nil();
  io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  editor_imgui_sdl2_init(window, gl_context);
  const char *glsl_version = "#version 330";
  editor_imgui_opengl3_init(glsl_version);

  editor_applyLevelSelection(0);
  editor_loadTileTextures();
  editor_reloadLevel(false, NULL);
  editor_loadEnemyPreviews();

  bool running = true;

  while (running)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      editor_imgui_sdl2_processEvent(&event);
      if (event.type == SDL_QUIT)
        running = false;
    }

    if (g_levelDirty)
    {
      editor_syncTilesToEngine();
      g_levelDirty = false;
    }

    editor_imgui_opengl3_newFrame();
    editor_imgui_sdl2_newFrame();
    igNewFrame();

    ImGuiViewport *viewport = igGetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;
    float previewWidth = workSize.x * 0.30f;
    if (previewWidth < 280.0f)
      previewWidth = 280.0f;
    ImVec2 previewSize = {previewWidth, workSize.y};
    ImVec2 editorPos = {workPos.x + previewSize.x, workPos.y};
    ImVec2 editorSize = {workSize.x - previewSize.x, workSize.y};

    igSetNextWindowPos(workPos, ImGuiCond_Always, (ImVec2){0.0f, 0.0f});
    igSetNextWindowSize(previewSize, ImGuiCond_Always);
    if (igBegin("Palette", NULL,
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoResize))
    {
      const float buttonSize = 72.0f;
      const float spacing = igGetStyle()->ItemSpacing.x;
      ImVec2 availRegion;
      igGetContentRegionAvail(&availRegion);
      float availableX = availRegion.x + spacing;
      int columns =
          (int)(availableX / (buttonSize + spacing > 0.0f ? buttonSize + spacing : buttonSize));
      if (columns < 1)
        columns = 1;
      const int minColumns = 4;
      if (columns < minColumns)
        columns = minColumns;

      if (g_editorMode == EDIT_MODE_WALLS)
      {
        if (igBeginChild_Str("##palette_scroll", (ImVec2){0.0f, 0.0f}, false,
                             ImGuiWindowFlags_HorizontalScrollbar))
        {
          ImVec2 tableSize = {0.0f, 0.0f};
          int tableColumns = columns;
          if (g_tileTextureCount > 0 && tableColumns > g_tileTextureCount)
            tableColumns = g_tileTextureCount;
          if (tableColumns < 1)
            tableColumns = 1;
          if (g_tileTextureCount > 0 &&
              igBeginTable("PaletteGrid##walls", tableColumns,
                           ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX |
                               ImGuiTableFlags_ScrollX,
                           tableSize, 0.0f))
          {
            int gridIndex = 0;
            for (int i = 0; i < g_tileTextureCount; ++i)
            {
              igPushID_Int(i);
              const TileTexture *slot = &g_tileTextures[i];
              if (slot->mapId <= 0)
              {
                igPopID();
                continue;
              }

              if ((gridIndex % tableColumns) == 0)
                igTableNextRow(0.0f, buttonSize + spacing * 2.0f);
              igTableSetColumnIndex(gridIndex % tableColumns);

              igBeginGroup();
              igText("%s", slot->name);
              ImVec4 bg = {0.0f, 0.0f, 0.0f, 0.0f};
              if (slot->mapId == g_selectedWallTileId)
                bg = (ImVec4){0.2f, 0.5f, 0.2f, 0.6f};

              ImVec2 imageSize = {buttonSize, buttonSize};
              char btnId[64];
              snprintf(btnId, sizeof(btnId), "wallbtn_%d", i);
              if (igImageButton(btnId, slot->texRef, imageSize,
                                (ImVec2){0.0f, 0.0f}, (ImVec2){1.0f, 1.0f}, bg,
                                (ImVec4){1, 1, 1, 1}))
              {
                g_selectedWallTileId = slot->mapId;
              }
              igEndGroup();
              gridIndex++;
              igPopID();
            }
            igEndTable();
          }
          igEndChild();
        }
      }
      else if (g_editorMode == EDIT_MODE_DECORATIONS)
      {
        if (igBeginChild_Str("##palette_scroll", (ImVec2){0.0f, 0.0f}, false,
                             ImGuiWindowFlags_HorizontalScrollbar))
        {
          ImVec2 tableSize = {0.0f, 0.0f};
          int tableColumns = columns;
          if (g_decorationTypeCount > 0 && tableColumns > g_decorationTypeCount)
            tableColumns = g_decorationTypeCount;
          if (tableColumns < 1)
            tableColumns = 1;
          if (g_decorationTypeCount > 0 &&
              igBeginTable("PaletteGrid##decor", tableColumns,
                           ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX |
                               ImGuiTableFlags_ScrollX,
                           tableSize, 0.0f))
          {
            int gridIndex = 0;
            for (int i = 0; i < g_decorationTypeCount; ++i)
            {
              igPushID_Int(i);
              const DecorationType *type = &g_decorationTypes[i];
              TileTexture *slot = editor_findTileByName(type->textureName);
              if (slot)
              {
                if ((gridIndex % tableColumns) == 0)
                  igTableNextRow(0.0f, buttonSize + spacing * 2.0f);
                igTableSetColumnIndex(gridIndex % tableColumns);

                igBeginGroup();
                igText("%s", type->label);
                ImVec4 bg = {0.0f, 0.0f, 0.0f, 0.0f};
                if (i == g_selectedDecorationType)
                  bg = (ImVec4){0.2f, 0.5f, 0.2f, 0.6f};
                char btnId[64];
                snprintf(btnId, sizeof(btnId), "decorbtn_%d", i);
                ImVec2 imageSize = {buttonSize, buttonSize};
                if (igImageButton(btnId, slot->texRef, imageSize,
                                  (ImVec2){0.0f, 0.0f}, (ImVec2){1.0f, 1.0f}, bg,
                                  (ImVec4){1, 1, 1, 1}))
                {
                  g_selectedDecorationType = i;
                }
                igEndGroup();
                gridIndex++;
              }
              igPopID();
            }
            igEndTable();
          }
          igEndChild();
        }
      }
      else if (g_editorMode == EDIT_MODE_PICKUPS)
      {
        if (igBeginChild_Str("##palette_scroll", (ImVec2){0.0f, 0.0f}, false,
                             ImGuiWindowFlags_HorizontalScrollbar))
        {
          ImVec2 tableSize = {0.0f, 0.0f};
          int tableColumns = columns;
          if (g_pickupTypeCount > 0 && tableColumns > g_pickupTypeCount)
            tableColumns = g_pickupTypeCount;
          if (tableColumns < 1)
            tableColumns = 1;
          if (g_pickupTypeCount > 0 &&
              igBeginTable("PaletteGrid##pickups", tableColumns,
                           ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX |
                               ImGuiTableFlags_ScrollX,
                           tableSize, 0.0f))
          {
            int gridIndex = 0;
            for (int i = 0; i < g_pickupTypeCount; ++i)
            {
              igPushID_Int(i);
              const PickupType *type = &g_pickupTypes[i];
              TileTexture *slot = editor_findTileByName(type->textureName);
              if (slot)
              {
                if ((gridIndex % tableColumns) == 0)
                  igTableNextRow(0.0f, buttonSize + spacing * 2.0f);
                igTableSetColumnIndex(gridIndex % tableColumns);

                igBeginGroup();
                igText("%s", type->label);
                ImVec4 bg = {0.0f, 0.0f, 0.0f, 0.0f};
                if (i == g_selectedPickupType)
                  bg = (ImVec4){0.2f, 0.5f, 0.2f, 0.6f};
                char btnId[64];
                snprintf(btnId, sizeof(btnId), "pickupbtn_%d", i);
                ImVec2 imageSize = {buttonSize, buttonSize};
                if (igImageButton(btnId, slot->texRef, imageSize,
                                  (ImVec2){0.0f, 0.0f}, (ImVec2){1.0f, 1.0f}, bg,
                                  (ImVec4){1, 1, 1, 1}))
                {
                  g_selectedPickupType = i;
                }
                igEndGroup();
                gridIndex++;
              }
              igPopID();
            }
            igEndTable();
          }
          igEndChild();
        }
      }
      else if (g_editorMode == EDIT_MODE_ENEMIES)
      {
        if (igBeginChild_Str("##palette_scroll", (ImVec2){0.0f, 0.0f}, false,
                             ImGuiWindowFlags_HorizontalScrollbar))
        {
          ImVec2 tableSize = {0.0f, 0.0f};
          int tableColumns = columns;
          if (g_enemyTypeCount > 0 && tableColumns > g_enemyTypeCount)
            tableColumns = g_enemyTypeCount;
          if (tableColumns < 1)
            tableColumns = 1;
          if (g_enemyTypeCount > 0 &&
              igBeginTable("PaletteGrid##enemies", tableColumns,
                           ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX |
                               ImGuiTableFlags_ScrollX,
                           tableSize, 0.0f))
          {
            int gridIndex = 0;
            for (int i = 0; i < g_enemyTypeCount; ++i)
            {
              igPushID_Int(i);
              const EnemyPreviewTexture *preview = &g_enemyPreviews[i];
              bool selected = (i == g_selectedEnemyType);
              ImVec4 bg = selected ? (ImVec4){0.2f, 0.5f, 0.2f, 0.6f}
                                   : (ImVec4){0.0f, 0.0f, 0.0f, 0.0f};
              ImVec4 tint = selected ? (ImVec4){1.0f, 1.0f, 1.0f, 1.0f}
                                     : (ImVec4){1.0f, 1.0f, 1.0f, 0.85f};
              ImVec2 size = {buttonSize, buttonSize};

              if ((gridIndex % tableColumns) == 0)
                igTableNextRow(0.0f, buttonSize + spacing * 2.0f);
              igTableSetColumnIndex(gridIndex % tableColumns);

              igBeginGroup();
              if (preview->id)
              {
                if (igImageButton("enemy_preview", preview->texRef, size,
                                  (ImVec2){0.0f, 0.0f}, (ImVec2){1.0f, 1.0f}, bg, tint))
                {
                  g_selectedEnemyType = i;
                }
              }
              else
              {
                if (igButton("Select##enemy", size))
                  g_selectedEnemyType = i;
              }
              igText("%s", g_enemyTypes[i].label);
              igEndGroup();
              gridIndex++;
              igPopID();
            }
            igEndTable();
          }
          igEndChild();
        }
      }
      else if (g_editorMode == EDIT_MODE_DECALS)
      {
        if (igBeginChild_Str("##palette_scroll", (ImVec2){0.0f, 0.0f}, false,
                             ImGuiWindowFlags_HorizontalScrollbar))
        {
          ImVec2 tableSize = {0.0f, 0.0f};
          int tableColumns = columns;
          if (g_decalTypeCount > 0 && tableColumns > g_decalTypeCount)
            tableColumns = g_decalTypeCount;
          if (tableColumns < 1)
            tableColumns = 1;
          if (g_decalTypeCount > 0 &&
              igBeginTable("PaletteGrid##decals", tableColumns,
                           ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX |
                               ImGuiTableFlags_ScrollX,
                           tableSize, 0.0f))
          {
            int gridIndex = 0;
            for (int i = 0; i < g_decalTypeCount; ++i)
            {
              igPushID_Int(i);
              const DecalType *type = &g_decalTypes[i];
              TileTexture *slot = editor_findTileByName(type->textureOffName);
              if (slot)
              {
                if ((gridIndex % tableColumns) == 0)
                  igTableNextRow(0.0f, buttonSize + spacing * 2.0f);
                igTableSetColumnIndex(gridIndex % tableColumns);

                igBeginGroup();
                igText("%s", type->label);
                ImVec4 bg = {0.0f, 0.0f, 0.0f, 0.0f};
                if (i == g_selectedDecalType)
                  bg = (ImVec4){0.2f, 0.5f, 0.2f, 0.6f};
                ImVec2 imageSize = {buttonSize, buttonSize};
                char btnId[64];
                snprintf(btnId, sizeof(btnId), "decalbtn_%d", i);
                if (igImageButton(btnId, slot->texRef, imageSize,
                                  (ImVec2){0.0f, 0.0f}, (ImVec2){1.0f, 1.0f}, bg,
                                  (ImVec4){1, 1, 1, 1}))
                {
                  g_selectedDecalType = i;
                }
                igEndGroup();
                gridIndex++;
              }
              igPopID();
            }
            igEndTable();
          }
          igEndChild();
        }
      }
      else if (g_editorMode == EDIT_MODE_TEXT)
      {
        if (igBeginChild_Str("##text_scroll", (ImVec2){0.0f, 0.0f}, false,
                             ImGuiWindowFlags_HorizontalScrollbar))
        {
          igText("Placed: %d", g_wallTextCount);
          if (g_wallTextCount == 0)
            igText("Click a wall tile to place text.");

          for (int i = 0; i < g_wallTextCount; ++i)
          {
            igPushID_Int(i);
            char label[160];
            const char *preview = g_wallTexts[i].text;
            if (!preview || preview[0] == '\0')
              preview = "(empty)";
            snprintf(label, sizeof(label), "%d: %.24s##walltext_%d", i, preview, i);
            bool selected = (i == g_selectedWallTextIndex);
            if (igSelectable_Bool(label, selected, 0, (ImVec2){0.0f, 0.0f}))
              g_selectedWallTextIndex = i;
            igPopID();
          }

          if (g_selectedWallTextIndex >= 0 &&
              g_selectedWallTextIndex < g_wallTextCount)
          {
            EditorWallText *text = &g_wallTexts[g_selectedWallTextIndex];
            igSeparator();
            igText("Selected text");
            if (igInputTextMultiline("Content##walltext", text->text,
                                     sizeof(text->text), (ImVec2){0.0f, 96.0f},
                                     ImGuiInputTextFlags_None, NULL, NULL))
            {
              g_entitiesDirty = true;
            }

            if (igSliderFloat("Scale##walltext", &text->scale, 0.1f, 1.0f,
                              "%.2f", 0))
            {
              g_entitiesDirty = true;
            }

            if (igInputInt("Font Size##walltext", &text->fontSize, 1, 2,
                           ImGuiInputTextFlags_None))
            {
              if (text->fontSize < 8)
                text->fontSize = 8;
              if (text->fontSize > 96)
                text->fontSize = 96;
              g_entitiesDirty = true;
            }

            if (igColorEdit4("Color##walltext", (float *)&text->color,
                             ImGuiColorEditFlags_Float))
            {
              g_entitiesDirty = true;
            }

            const char *facingLabel =
                (text->facingIndex >= 0 && text->facingIndex < g_decalFacingCount)
                    ? g_decalFacingLabels[text->facingIndex]
                    : g_decalFacingLabels[0];
            if (igBeginCombo("Facing##walltext", facingLabel, 0))
            {
              for (int i = 0; i < g_decalFacingCount; ++i)
              {
                bool selected = (text->facingIndex == i);
                if (igSelectable_Bool(g_decalFacingLabels[i], selected, 0,
                                      (ImVec2){0.0f, 0.0f}))
                {
                  editor_applyWallTextFacing(text, i);
                  g_entitiesDirty = true;
                }
                if (selected)
                  igSetItemDefaultFocus();
              }
              igEndCombo();
            }

            int tileCoords[2] = {text->tileX, text->tileY};
            if (igInputInt2("Tile##walltext", tileCoords,
                            ImGuiInputTextFlags_None))
            {
              if (tileCoords[0] < 0)
                tileCoords[0] = 0;
              else if (tileCoords[0] >= MAP_WIDTH)
                tileCoords[0] = MAP_WIDTH - 1;
              if (tileCoords[1] < 0)
                tileCoords[1] = 0;
              else if (tileCoords[1] >= MAP_HEIGHT)
                tileCoords[1] = MAP_HEIGHT - 1;
              text->tileX = tileCoords[0];
              text->tileY = tileCoords[1];
              text->x = (float)text->tileX + 0.5f;
              text->y = (float)text->tileY + 0.5f;
              g_entitiesDirty = true;
            }

            if (igButton("Delete Text", (ImVec2){0, 0}))
            {
              editor_removeWallTextAtIndex(g_selectedWallTextIndex);
              g_selectedWallTextIndex = -1;
            }
          }

          igEndChild();
        }
      }
      else if (g_editorMode == EDIT_MODE_SPAWN)
      {
        igTextWrapped("Spawn mode:\n  Left click to move the spawn point\n  Right click to rotate the facing direction");
      }
      else
      {
        igText("Select enemy type in the Level Editing panel.");
      }
      igEnd();
    }

    igSetNextWindowPos(editorPos, ImGuiCond_Always, (ImVec2){0.0f, 0.0f});
    igSetNextWindowSize(editorSize, ImGuiCond_Always);
    if (igBegin("Level Editing", NULL,
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoResize))
    {
      const LevelInfo *currentLevel = editor_getCurrentLevel();
      igText("Level");
      igSameLine(0.0f, 6.0f);
      if (igBeginCombo("##level_combo", currentLevel->label, 0))
      {
        for (int i = 0; i < g_levelInfoCount; ++i)
        {
          bool selected = (i == g_selectedLevelIndex);
          if (igSelectable_Bool(g_levelInfos[i].label, selected, 0,
                                (ImVec2){0.0f, 0.0f}))
          {
            editor_changeLevel(i);
            currentLevel = editor_getCurrentLevel();
          }
          if (selected)
            igSetItemDefaultFocus();
        }
        igEndCombo();
      }
      igSameLine(0.0f, 6.0f);
      if (igButton("Reload Level##level", (ImVec2){0, 0}))
      {
        editor_reloadLevel(true, "Reloaded %s.");
      }

      igSeparator();
      igText("File");
      if (igButton("Save Level", (ImVec2){0, 0}))
      {
        editor_saveMap();
        editor_saveEntities();
        currentLevel = editor_getCurrentLevel();
        editor_setStatus("Saved %s.", currentLevel->label);
      }
      bool showedInline = false;
      if (g_mapNeedsSave || g_entitiesDirty)
      {
        igSameLine(0.0f, 12.0f);
        igTextColored((ImVec4){0.95f, 0.7f, 0.2f, 1.0f}, "Unsaved changes");
        showedInline = true;
      }
      if (editor_statusActive())
      {
        if (!showedInline)
        {
          igSameLine(0.0f, 12.0f);
        }
        igTextColored(g_statusColor, "%s", g_statusMessage);
      }

      igSeparator();
      igText("Editing Mode");
      igRadioButton_IntPtr("Walls", (int *)&g_editorMode, EDIT_MODE_WALLS);
      igSameLine(0.0f, 8.0f);
      igRadioButton_IntPtr("Decorations", (int *)&g_editorMode, EDIT_MODE_DECORATIONS);
      igSameLine(0.0f, 8.0f);
      igRadioButton_IntPtr("Pickups", (int *)&g_editorMode, EDIT_MODE_PICKUPS);
      igSameLine(0.0f, 8.0f);
      igRadioButton_IntPtr("Enemies", (int *)&g_editorMode, EDIT_MODE_ENEMIES);
      igSameLine(0.0f, 8.0f);
      igRadioButton_IntPtr("Decals", (int *)&g_editorMode, EDIT_MODE_DECALS);
      igSameLine(0.0f, 8.0f);
      igRadioButton_IntPtr("Text", (int *)&g_editorMode, EDIT_MODE_TEXT);
      igSameLine(0.0f, 8.0f);
      igRadioButton_IntPtr("Spawn", (int *)&g_editorMode, EDIT_MODE_SPAWN);

      igSeparator();
      igText("Floor & Ceiling");
      editor_drawFloorCeilingControls();
      igSeparator();

      int totalEntities = editor_totalEntityCount();
      igText("Sprite slots used: %d / %d", totalEntities, NUM_SPRITES);
      if (totalEntities >= NUM_SPRITES)
        igTextColored((ImVec4){0.9f, 0.4f, 0.2f, 1.0f}, "Sprite capacity reached - remove entries before adding more.");
      igSeparator();
      igText("Player Spawn");
      igText("Current: (%.2f, %.2f)", g_playerSpawnPos.x, g_playerSpawnPos.y);
      float spawnPos[2] = {g_playerSpawnPos.x, g_playerSpawnPos.y};
      if (igInputFloat2("Position##spawn", spawnPos, "%.2f", 0))
      {
        editor_setSpawnPosition(spawnPos[0], spawnPos[1]);
      }
      float spawnDir = g_playerSpawnDirDegrees;
      if (igSliderFloat("Direction##spawn", &spawnDir, 0.0f, 360.0f, "%.1f deg", 0))
      {
        editor_setSpawnDirection(spawnDir);
      }
      igTextDisabled("Use Spawn mode to click in the grid.");
      igSeparator();

      if (g_editorMode == EDIT_MODE_WALLS)
      {
        const char *wallName =
            (g_selectedWallTileId > 0 && g_selectedWallTileId <= NUM_WALL_TEXTURES)
                ? wallTextures[g_selectedWallTileId - 1].name
                : "Empty";
        igText("Active wall: %s (ID %d)", wallName, g_selectedWallTileId);
        if (igButton("Erase (0)", (ImVec2){0, 0}))
          g_selectedWallTileId = 0;
        igSameLine(0.0f, 6.0f);
        if (igButton("Fill", (ImVec2){0, 0}))
          editor_fillTiles(g_selectedWallTileId);
      }
      else if (g_editorMode == EDIT_MODE_DECORATIONS)
      {
        const DecorationType *brush = &g_decorationTypes[g_selectedDecorationType];
        igText("Active decoration: %s", brush->label);
        if (igCollapsingHeader_TreeNodeFlags("Decorations##section", ImGuiTreeNodeFlags_DefaultOpen))
        {
          igPushID_Str("decor_section");
          igText("Placed: %d", g_decorationCount);
          if (g_decorationCount == 0)
            igText("Click the grid to place a decoration.");
          for (int i = 0; i < g_decorationCount; ++i)
          {
            igPushID_Int(i);
            const DecorationType *type = &g_decorationTypes[g_decorations[i].typeIndex];
            char label[128];
            snprintf(label, sizeof(label), "%s (%.1f, %.1f)##decor_%d", type->label,
                     g_decorations[i].x, g_decorations[i].y, i);
            bool selected = (i == g_selectedDecorationIndex);
            if (igSelectable_Bool(label, selected, 0, (ImVec2){0.0f, 0.0f}))
              g_selectedDecorationIndex = i;
            igPopID();
          }

          if (g_selectedDecorationIndex >= 0 &&
              g_selectedDecorationIndex < g_decorationCount)
          {
            EditorDecoration *decor = &g_decorations[g_selectedDecorationIndex];
            const DecorationType *type = &g_decorationTypes[decor->typeIndex];
            igSeparator();
            igText("Selected decoration");
            igText("Position: (%.2f, %.2f)", decor->x, decor->y);
            if (igBeginCombo("Type##decor", type->label, 0))
            {
              for (int i = 0; i < g_decorationTypeCount; ++i)
              {
                bool selected = (decor->typeIndex == i);
                if (igSelectable_Bool(g_decorationTypes[i].label, selected, 0,
                                      (ImVec2){0.0f, 0.0f}))
                {
                  decor->typeIndex = i;
                  g_entitiesDirty = true;
                }
                if (selected)
                  igSetItemDefaultFocus();
              }
              igEndCombo();
            }

            float scale = decor->scale;
            if (igSliderFloat("Scale##decor", &scale, 0.4f, 2.5f, "%.2f", 0))
            {
              decor->scale = scale;
              g_entitiesDirty = true;
            }
            igSameLine(0.0f, 6.0f);
            if (igButton("Reset Scale##decor", (ImVec2){0, 0}))
            {
              decor->scale = g_decorationTypes[decor->typeIndex].defaultScale;
              g_entitiesDirty = true;
            }
            if (igButton("Delete Decoration", (ImVec2){0, 0}))
            {
              editor_removeDecorationAtIndex(g_selectedDecorationIndex);
              g_selectedDecorationIndex = -1;
            }
          }
          igPopID();
        }
      }
      else if (g_editorMode == EDIT_MODE_PICKUPS)
      {
        const PickupType *brush = &g_pickupTypes[g_selectedPickupType];
        igText("Active pickup: %s", brush->label);
        if (igCollapsingHeader_TreeNodeFlags("Pickups##section", ImGuiTreeNodeFlags_DefaultOpen))
        {
          igPushID_Str("pickup_section");
          igText("Placed: %d", g_pickupCount);
          if (g_pickupCount == 0)
            igText("Click the grid to place a pickup.");
          for (int i = 0; i < g_pickupCount; ++i)
          {
            igPushID_Int(i);
            const PickupType *type = &g_pickupTypes[g_pickups[i].typeIndex];
            char label[128];
            snprintf(label, sizeof(label), "%s (%.1f, %.1f)##pickup_%d", type->label,
                     g_pickups[i].x, g_pickups[i].y, i);
            bool selected = (i == g_selectedPickupIndex);
            if (igSelectable_Bool(label, selected, 0, (ImVec2){0.0f, 0.0f}))
              g_selectedPickupIndex = i;
            igPopID();
          }

          if (g_selectedPickupIndex >= 0 && g_selectedPickupIndex < g_pickupCount)
          {
            EditorPickup *pickup = &g_pickups[g_selectedPickupIndex];
            const PickupType *type = &g_pickupTypes[pickup->typeIndex];
            igSeparator();
            igText("Selected pickup");
            igText("Position: (%.2f, %.2f)", pickup->x, pickup->y);
            if (igBeginCombo("Type##pickup", type->label, 0))
            {
              for (int i = 0; i < g_pickupTypeCount; ++i)
              {
                bool selected = (pickup->typeIndex == i);
                if (igSelectable_Bool(g_pickupTypes[i].label, selected, 0,
                                      (ImVec2){0.0f, 0.0f}))
                {
                  pickup->typeIndex = i;
                  g_entitiesDirty = true;
                }
                if (selected)
                  igSetItemDefaultFocus();
              }
              igEndCombo();
            }

            float scale = pickup->scale;
            if (igSliderFloat("Scale##pickup", &scale, 0.4f, 2.5f, "%.2f", 0))
            {
              pickup->scale = scale;
              g_entitiesDirty = true;
            }
            igSameLine(0.0f, 6.0f);
            if (igButton("Reset Scale##pickup", (ImVec2){0, 0}))
            {
              pickup->scale = g_pickupTypes[pickup->typeIndex].defaultScale;
              g_entitiesDirty = true;
            }
            if (igButton("Delete Pickup", (ImVec2){0, 0}))
            {
              editor_removePickupAtIndex(g_selectedPickupIndex);
              g_selectedPickupIndex = -1;
            }
          }
          igPopID();
        }
      }
      else if (g_editorMode == EDIT_MODE_ENEMIES)
      {
        const EnemyType *type = &g_enemyTypes[g_selectedEnemyType];
        igText("Active enemy: %s", type->label);

        if (igCollapsingHeader_TreeNodeFlags("Enemies##section", ImGuiTreeNodeFlags_DefaultOpen))
        {
          igPushID_Str("enemy_section");
          igText("Placed: %d", g_enemyCount);
          if (g_enemyCount == 0)
            igText("Click the grid to place an enemy.");
          for (int i = 0; i < g_enemyCount; ++i)
          {
            igPushID_Int(i);
            char label[64];
            snprintf(label, sizeof(label), "Enemy %d (%.1f, %.1f)##enemy_%d", i,
                     g_enemies[i].x, g_enemies[i].y, i);
            bool selected = (i == g_selectedEnemyIndex);
            if (igSelectable_Bool(label, selected, 0, (ImVec2){0.0f, 0.0f}))
              g_selectedEnemyIndex = i;
            igPopID();
          }

          if (g_selectedEnemyIndex >= 0 && g_selectedEnemyIndex < g_enemyCount)
          {
            EditorEnemy *enemy = &g_enemies[g_selectedEnemyIndex];
            igSeparator();
            igText("Selected enemy");
            igText("Position: (%.2f, %.2f)", enemy->x, enemy->y);

            float scale = enemy->scale;
            if (igSliderFloat("Scale##enemy", &scale, 0.5f, 3.0f, "%.2f", 0))
            {
              enemy->scale = scale;
              g_entitiesDirty = true;
            }

            int health = enemy->health;
            if (igSliderInt("Health", &health, 1, 500, "%d", 0))
            {
              enemy->health = health;
              g_entitiesDirty = true;
            }

            if (igBeginCombo("Animation##enemy", enemy->animation, 0))
            {
              for (int i = 0; i < g_enemyTypeCount; ++i)
              {
                bool selected = (strcmp(enemy->animation,
                                        g_enemyTypes[i].animation) == 0);
                if (igSelectable_Bool(g_enemyTypes[i].label, selected, 0,
                                      (ImVec2){0.0f, 0.0f}))
                {
                  snprintf(enemy->animation, sizeof(enemy->animation), "%s",
                           g_enemyTypes[i].animation);
                  g_entitiesDirty = true;
                }
                if (selected)
                  igSetItemDefaultFocus();
              }
              igEndCombo();
            }

            if (igButton("Apply Type Defaults", (ImVec2){0, 0}))
            {
              const EnemyType *defaults = NULL;
              for (int i = 0; i < g_enemyTypeCount; ++i)
              {
                if (strcmp(enemy->animation, g_enemyTypes[i].animation) == 0)
                {
                  defaults = &g_enemyTypes[i];
                  break;
                }
              }
              if (!defaults)
                defaults = &g_enemyTypes[g_selectedEnemyType];
              enemy->scale = defaults->defaultScale;
              enemy->health = defaults->defaultHealth;
              g_entitiesDirty = true;
            }

            if (igButton("Delete Selected", (ImVec2){0, 0}))
            {
              editor_removeEnemyAtIndex(g_selectedEnemyIndex);
              g_selectedEnemyIndex = -1;
            }
          }
          igPopID();
        }
      }
      else if (g_editorMode == EDIT_MODE_DECALS)
      {
        const DecalType *type = &g_decalTypes[g_selectedDecalType];
        igText("Active decal: %s", type->label);
        if (igCollapsingHeader_TreeNodeFlags("Decals##section", ImGuiTreeNodeFlags_DefaultOpen))
        {
          igPushID_Str("decal_section");
          igText("Placed: %d", g_decalCount);
          if (g_decalCount == 0)
            igText("Click the grid to place a decal.");
          for (int i = 0; i < g_decalCount; ++i)
          {
            igPushID_Int(i);
            const DecalType *rowType = &g_decalTypes[g_decals[i].typeIndex];
            const char *facingText =
                (g_decals[i].facingIndex >= 0 &&
                 g_decals[i].facingIndex < g_decalFacingCount)
                    ? g_decalFacingLabels[g_decals[i].facingIndex]
                    : "?";
            char label[128];
            snprintf(label, sizeof(label), "%s [%s] (door %d,%d)##decal_%d",
                     rowType->label, facingText, g_decals[i].doorX,
                     g_decals[i].doorY, i);
            bool selected = (i == g_selectedDecalIndex);
            if (igSelectable_Bool(label, selected, 0, (ImVec2){0.0f, 0.0f}))
              g_selectedDecalIndex = i;
            igPopID();
          }

          if (g_selectedDecalIndex >= 0 && g_selectedDecalIndex < g_decalCount)
          {
            EditorDecal *decal = &g_decals[g_selectedDecalIndex];
            const DecalType *selectedType = &g_decalTypes[decal->typeIndex];
            igSeparator();
            igText("Selected decal");
            igText("Position: (%.2f, %.2f)", decal->x, decal->y);
            if (igBeginCombo("Type##decal", selectedType->label, 0))
            {
              for (int i = 0; i < g_decalTypeCount; ++i)
              {
                bool selected = (decal->typeIndex == i);
                if (igSelectable_Bool(g_decalTypes[i].label, selected, 0,
                                      (ImVec2){0.0f, 0.0f}))
                {
                  decal->typeIndex = i;
                  g_entitiesDirty = true;
                }
                if (selected)
                  igSetItemDefaultFocus();
              }
              igEndCombo();
            }

            const char *facingLabel =
                (decal->facingIndex >= 0 && decal->facingIndex < g_decalFacingCount)
                    ? g_decalFacingLabels[decal->facingIndex]
                    : "Unknown";
            if (igBeginCombo("Facing##decal", facingLabel, 0))
            {
              for (int i = 0; i < g_decalFacingCount; ++i)
              {
                bool selected = (decal->facingIndex == i);
                if (igSelectable_Bool(g_decalFacingLabels[i], selected, 0,
                                      (ImVec2){0.0f, 0.0f}))
                {
                  editor_applyDecalFacing(decal, i, true);
                  g_entitiesDirty = true;
                }
                if (selected)
                  igSetItemDefaultFocus();
              }
              igEndCombo();
            }

            int doorCoords[2] = {decal->doorX, decal->doorY};
            if (igInputInt2("Door Tile##decal", doorCoords, ImGuiInputTextFlags_None))
            {
              if (doorCoords[0] < 0)
                doorCoords[0] = 0;
              else if (doorCoords[0] >= MAP_WIDTH)
                doorCoords[0] = MAP_WIDTH - 1;
              if (doorCoords[1] < 0)
                doorCoords[1] = 0;
              else if (doorCoords[1] >= MAP_HEIGHT)
                doorCoords[1] = MAP_HEIGHT - 1;
              decal->doorX = doorCoords[0];
              decal->doorY = doorCoords[1];
              g_entitiesDirty = true;
            }

            if (igButton("Door In Front", (ImVec2){0, 0}))
            {
              editor_applyDecalFacing(decal, decal->facingIndex, true);
              g_entitiesDirty = true;
            }

            if (igButton("Assign Door From Map", (ImVec2){0, 0}))
            {
              g_pendingDoorAssignmentIndex = g_selectedDecalIndex;
              editor_setStatus("Click a wall tile to assign lever target.");
            }

            if (g_pendingDoorAssignmentIndex == g_selectedDecalIndex)
            {
              igSameLine(0.0f, 8.0f);
              igTextColored((ImVec4){0.8f, 0.7f, 0.2f, 1.0f},
                            "Awaiting door selection...");
            }

            if (igButton("Delete Decal", (ImVec2){0, 0}))
            {
              editor_removeDecalAtIndex(g_selectedDecalIndex);
              g_selectedDecalIndex = -1;
            }
          }
          igPopID();
        }
      }

      igSeparator();

      const float tileSize = 28.0f;
      ImVec2 canvasSize = {MAP_WIDTH * tileSize, MAP_HEIGHT * tileSize};
      ImVec2 origin;
      igGetCursorScreenPos(&origin);
      ImDrawList *drawList = igGetWindowDrawList();

      for (int y = 0; y < MAP_HEIGHT; ++y)
      {
        for (int x = 0; x < MAP_WIDTH; ++x)
        {
          int tile = g_levelTiles[x][y];
          ImVec2 p0 = {origin.x + x * tileSize, origin.y + y * tileSize};
          ImVec2 p1 = {p0.x + tileSize, p0.y + tileSize};
          ImU32 fillColor = (tile > 0) ? EDITOR_COL32(180, 180, 120, 255)
                                       : EDITOR_COL32(45, 45, 45, 255);
          ImDrawList_AddRectFilled(drawList, p0, p1, fillColor, 0.0f, 0);
          ImDrawList_AddRect(drawList, p0, p1, EDITOR_COL32(70, 70, 70, 255),
                             0.0f, 0, 1.0f);
        }
      }

      for (int i = 0; i < g_decorationCount; ++i)
      {
        const EditorDecoration *decor = &g_decorations[i];
        ImVec2 center = {origin.x + decor->x * tileSize,
                         origin.y + decor->y * tileSize};
        float half = tileSize * 0.25f;
        ImVec2 p0 = {center.x - half, center.y - half};
        ImVec2 p1 = {center.x + half, center.y + half};
        ImU32 fill =
            (i == g_selectedDecorationIndex)
                ? EDITOR_COL32(80, 200, 120, 255)
                : EDITOR_COL32(60, 150, 100, 220);
        ImDrawList_AddRectFilled(drawList, p0, p1, fill, 4.0f, 0);
        ImDrawList_AddRect(drawList, p0, p1, EDITOR_COL32(15, 40, 25, 255), 4.0f,
                           0, 1.5f);
      }

      for (int i = 0; i < g_pickupCount; ++i)
      {
        const EditorPickup *pickup = &g_pickups[i];
        ImVec2 center = {origin.x + pickup->x * tileSize,
                         origin.y + pickup->y * tileSize};
        float half = tileSize * 0.24f;
        ImVec2 pTop = {center.x, center.y - half};
        ImVec2 pRight = {center.x + half, center.y};
        ImVec2 pBottom = {center.x, center.y + half};
        ImVec2 pLeft = {center.x - half, center.y};
        ImU32 fill =
            (i == g_selectedPickupIndex)
                ? EDITOR_COL32(220, 185, 70, 255)
                : EDITOR_COL32(200, 165, 60, 220);
        ImDrawList_AddQuadFilled(drawList, pTop, pRight, pBottom, pLeft, fill);
        ImDrawList_AddQuad(drawList, pTop, pRight, pBottom, pLeft,
                           EDITOR_COL32(40, 25, 5, 255), 1.5f);
      }

      for (int i = 0; i < g_decalCount; ++i)
      {
        const EditorDecal *decal = &g_decals[i];
        ImVec2 center = {origin.x + decal->x * tileSize,
                         origin.y + decal->y * tileSize};
        float half = tileSize * 0.20f;
        ImVec2 p0 = {center.x - half, center.y - half};
        ImVec2 p2 = {center.x + half, center.y + half};
        ImU32 fill =
            (i == g_selectedDecalIndex)
                ? EDITOR_COL32(120, 180, 250, 255)
                : EDITOR_COL32(90, 140, 220, 220);
        ImDrawList_AddRectFilled(drawList, p0, p2, fill, 4.0f, 0);
        ImDrawList_AddRect(drawList, p0, p2, EDITOR_COL32(20, 50, 120, 255),
                           4.0f, 0, 1.5f);
      }

      for (int i = 0; i < g_wallTextCount; ++i)
      {
        const EditorWallText *text = &g_wallTexts[i];
        ImVec2 center = {origin.x + text->x * tileSize,
                         origin.y + text->y * tileSize};
        float half = tileSize * 0.18f;
        ImVec2 p0 = {center.x - half, center.y - half};
        ImVec2 p1 = {center.x + half, center.y + half};
        ImU32 fill = (i == g_selectedWallTextIndex)
                         ? EDITOR_COL32(200, 200, 110, 255)
                         : EDITOR_COL32(170, 170, 90, 220);
        ImDrawList_AddRectFilled(drawList, p0, p1, fill, 3.0f, 0);
        ImDrawList_AddRect(drawList, p0, p1, EDITOR_COL32(60, 60, 20, 255),
                           3.0f, 0, 1.2f);
        ImVec2 facingDir = {g_decalFacingVectors[text->facingIndex][0] * half,
                            g_decalFacingVectors[text->facingIndex][1] * half};
        ImVec2 arrowEnd = {center.x + facingDir.x, center.y + facingDir.y};
        ImDrawList_AddLine(drawList, center, arrowEnd,
                           EDITOR_COL32(40, 40, 10, 255), 2.0f);
      }

      for (int i = 0; i < g_enemyCount; ++i)
      {
        const EditorEnemy *enemy = &g_enemies[i];
        ImVec2 center = {origin.x + enemy->x * tileSize,
                         origin.y + enemy->y * tileSize};
        float radius = tileSize * 0.3f;
        ImU32 fill =
            (i == g_selectedEnemyIndex)
                ? EDITOR_COL32(230, 90, 70, 255)
                : EDITOR_COL32(200, 60, 60, 220);
        ImDrawList_AddCircleFilled(drawList, center, radius, fill, 16);
        ImDrawList_AddCircle(drawList, center, radius,
                             EDITOR_COL32(20, 20, 20, 255), 16, 2.0f);
      }

      ImVec2 spawnCenter = {origin.x + g_playerSpawnPos.x * tileSize,
                            origin.y + g_playerSpawnPos.y * tileSize};
      float spawnRadius = tileSize * 0.32f;
      ImU32 spawnFill =
          (g_editorMode == EDIT_MODE_SPAWN)
              ? EDITOR_COL32(120, 200, 255, 235)
              : EDITOR_COL32(90, 150, 220, 200);
      ImU32 spawnOutline = EDITOR_COL32(15, 70, 140, 255);
      ImDrawList_AddCircleFilled(drawList, spawnCenter, spawnRadius, spawnFill, 24);
      ImDrawList_AddCircle(drawList, spawnCenter, spawnRadius, spawnOutline, 24, 2.0f);
      float dirRad =
          (float)(g_playerSpawnDirDegrees * (float)M_PI / 180.0f);
      ImVec2 dirVec = {cosf(dirRad), -sinf(dirRad)};
      ImVec2 arrowHead = {spawnCenter.x + dirVec.x * spawnRadius,
                          spawnCenter.y + dirVec.y * spawnRadius};
      ImVec2 arrowTail = {spawnCenter.x - dirVec.x * (spawnRadius * 0.4f),
                          spawnCenter.y - dirVec.y * (spawnRadius * 0.4f)};
      ImDrawList_AddLine(drawList, arrowTail, arrowHead, spawnOutline, 2.2f);
      ImDrawList_AddCircleFilled(drawList, arrowHead, spawnRadius * 0.18f,
                                 spawnOutline, 12);

      igInvisibleButton("grid_canvas", canvasSize, 0);
      bool hovered = igIsItemHovered(ImGuiHoveredFlags_None);
      ImGuiIO *io = igGetIO_Nil();

      if (hovered)
      {
        int tileX = (int)((io->MousePos.x - origin.x) / tileSize);
        int tileY = (int)((io->MousePos.y - origin.y) / tileSize);
        if (tileX >= 0 && tileX < MAP_WIDTH && tileY >= 0 && tileY < MAP_HEIGHT)
        {
          ImVec2 highlight0 = {origin.x + tileX * tileSize,
                               origin.y + tileY * tileSize};
          ImVec2 highlight1 = {highlight0.x + tileSize,
                               highlight0.y + tileSize};
          ImDrawList_AddRect(drawList, highlight0, highlight1,
                             EDITOR_COL32(210, 210, 60, 255), 0.0f, 0, 2.0f);

          switch (g_editorMode)
          {
          case EDIT_MODE_WALLS:
            if (io->MouseDown[0])
            {
              if (g_levelTiles[tileX][tileY] != g_selectedWallTileId)
              {
                g_levelTiles[tileX][tileY] = g_selectedWallTileId;
                g_levelDirty = true;
                g_mapNeedsSave = true;
              }
            }
            else if (io->MouseDown[1])
            {
              if (g_levelTiles[tileX][tileY] != 0)
              {
                g_levelTiles[tileX][tileY] = 0;
                g_levelDirty = true;
                g_mapNeedsSave = true;
              }
            }
            break;
          case EDIT_MODE_DECORATIONS:
          {
            int existing = editor_findDecorationAtTile(tileX, tileY);
            if (io->MouseClicked[0])
            {
              if (existing >= 0)
              {
                g_selectedDecorationIndex = existing;
              }
              else
              {
                EditorDecoration *created =
                    editor_addDecoration((float)tileX + 0.5f,
                                         (float)tileY + 0.5f,
                                         g_selectedDecorationType);
                if (created)
                  g_selectedDecorationIndex = g_decorationCount - 1;
              }
            }
            if (io->MouseClicked[1] && existing >= 0)
            {
              editor_removeDecorationAtIndex(existing);
              if (g_selectedDecorationIndex == existing)
                g_selectedDecorationIndex = -1;
            }
          }
          break;
          case EDIT_MODE_PICKUPS:
          {
            int existing = editor_findPickupAtTile(tileX, tileY);
            if (io->MouseClicked[0])
            {
              if (existing >= 0)
              {
                g_selectedPickupIndex = existing;
              }
              else
              {
                EditorPickup *created =
                    editor_addPickup((float)tileX + 0.5f,
                                     (float)tileY + 0.5f,
                                     g_selectedPickupType);
                if (created)
                  g_selectedPickupIndex = g_pickupCount - 1;
              }
            }
            if (io->MouseClicked[1] && existing >= 0)
            {
              editor_removePickupAtIndex(existing);
              if (g_selectedPickupIndex == existing)
                g_selectedPickupIndex = -1;
            }
          }
          break;
          case EDIT_MODE_ENEMIES:
          {
            int existing = editor_enemyIndexAtTile(tileX, tileY);
            if (io->MouseClicked[0])
            {
              if (existing >= 0)
              {
                g_selectedEnemyIndex = existing;
              }
              else
              {
                const EnemyType *type = &g_enemyTypes[g_selectedEnemyType];
                EditorEnemy *created =
                    editor_addEnemy((float)tileX + 0.5f,
                                    (float)tileY + 0.5f, type);
                if (created)
                  g_selectedEnemyIndex = g_enemyCount - 1;
              }
            }
            if (io->MouseClicked[1] && existing >= 0)
            {
              editor_removeEnemyAtIndex(existing);
              if (g_selectedEnemyIndex == existing)
                g_selectedEnemyIndex = -1;
            }
          }
          break;
      case EDIT_MODE_DECALS:
      {
        if (g_pendingDoorAssignmentIndex >= 0)
        {
              if (g_levelTiles[tileX][tileY] > 0)
              {
                ImVec2 doorP0 = {origin.x + tileX * tileSize,
                                  origin.y + tileY * tileSize};
                ImVec2 doorP1 = {doorP0.x + tileSize, doorP0.y + tileSize};
                ImDrawList_AddRect(drawList, doorP0, doorP1,
                                   EDITOR_COL32(255, 200, 80, 255), 0.0f, 0,
                                   3.0f);
                ImDrawList_AddRectFilled(drawList, doorP0, doorP1,
                                         EDITOR_COL32(255, 200, 80, 60), 0.0f,
                                         0);
              }
              else
              {
                ImVec2 invalidP0 = {origin.x + tileX * tileSize,
                                     origin.y + tileY * tileSize};
                ImVec2 invalidP1 = {invalidP0.x + tileSize,
                                    invalidP0.y + tileSize};
                ImDrawList_AddRect(drawList, invalidP0, invalidP1,
                                   EDITOR_COL32(200, 60, 60, 255), 0.0f, 0,
                                   3.0f);
              }

              if (io->MouseClicked[0])
              {
                if (g_levelTiles[tileX][tileY] > 0)
                {
                  EditorDecal *target =
                      &g_decals[g_pendingDoorAssignmentIndex];
                  target->doorX = tileX;
                  target->doorY = tileY;
                  g_entitiesDirty = true;
                  editor_setStatus("Lever target set to (%d,%d).", tileX,
                                   tileY);
                  g_pendingDoorAssignmentIndex = -1;
                }
                else
                {
                  editor_setErrorStatus(
                      "Selected tile is not a wall; choose a solid tile.");
                }
              }
              else if (io->MouseClicked[1])
              {
                g_pendingDoorAssignmentIndex = -1;
                editor_setStatus("Door assignment cancelled.");
              }
            }
            else
            {
              int existing = editor_findDecalAtTile(tileX, tileY);
              if (io->MouseClicked[0])
              {
                if (existing >= 0)
                {
                  g_selectedDecalIndex = existing;
                }
                else
                {
                  EditorDecal *created =
                      editor_addDecal((float)tileX + 0.5f,
                                      (float)tileY + 0.5f, g_selectedDecalType);
                  if (created)
                    g_selectedDecalIndex = g_decalCount - 1;
                }
              }
              if (io->MouseClicked[1] && existing >= 0)
              {
                editor_removeDecalAtIndex(existing);
                if (g_selectedDecalIndex == existing)
                  g_selectedDecalIndex = -1;
              }
            }
      }
      break;
      case EDIT_MODE_TEXT:
      {
        int existing = editor_findWallTextAtTile(tileX, tileY);
        if (io->MouseClicked[0])
        {
          if (existing >= 0)
          {
            g_selectedWallTextIndex = existing;
          }
          else
          {
            EditorWallText *created =
                editor_addWallText((float)tileX + 0.5f,
                                   (float)tileY + 0.5f);
            if (created)
              g_selectedWallTextIndex = g_wallTextCount - 1;
          }
        }
        if (io->MouseClicked[1] && existing >= 0)
        {
          editor_removeWallTextAtIndex(existing);
          if (g_selectedWallTextIndex == existing)
            g_selectedWallTextIndex = -1;
        }
      }
      break;
      case EDIT_MODE_SPAWN:
      {
        if (io->MouseClicked[0])
        {
          float newX = (float)tileX + 0.5f;
              float newY = (float)tileY + 0.5f;
              editor_setSpawnPosition(newX, newY);
            }
            if (io->MouseDown[1])
            {
              float pointerX = (io->MousePos.x - origin.x) / tileSize;
              float pointerY = (io->MousePos.y - origin.y) / tileSize;
              float deltaX = pointerX - g_playerSpawnPos.x;
              float deltaY = g_playerSpawnPos.y - pointerY;
              if (fabsf(deltaX) > 0.001f || fabsf(deltaY) > 0.001f)
              {
                float angleDeg =
                    (float)(atan2f(deltaY, deltaX) * (180.0f / (float)M_PI));
                editor_setSpawnDirection(angleDeg);
              }
            }
          }
          break;
          }
        }
      }

      igDummy((ImVec2){canvasSize.x, canvasSize.y});

      igEnd();
    }

    igRender();
    glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
    glClearColor(0.1f, 0.12f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    editor_imgui_opengl3_renderDrawData(igGetDrawData());
    SDL_GL_SwapWindow(window);
  }

  editor_freeTileTextures();
  editor_clearEnemyPreviews();
  IMG_Quit();

  editor_imgui_opengl3_shutdown();
  editor_imgui_sdl2_shutdown();
  igDestroyContext(NULL);

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
