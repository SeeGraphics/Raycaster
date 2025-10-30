#include "entities.h"
#include "animation.h"
#include "types.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Sprite worldSprites[NUM_SPRITES];
static i32 worldSpriteCount = 0;
static int worldInitialized = 0;

static Animation *animation_from_name(const char *name);

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
  }
}

static void entities_populateDefaults(void)
{
  worldSpriteCount = 0;

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
      {15.5f, 13.5f, 1.35f, 200, "DEMON_WALK"},
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

  worldSpriteCount = 0;
  int result = 0;

  if (parse_entity_array(buffer, "decorations", parse_decoration_object) != 0)
    result = -1;

  if (result == 0 &&
      parse_entity_array(buffer, "pickups", parse_pickup_object) != 0)
    result = -1;

  if (result == 0 &&
      parse_entity_array(buffer, "enemies", parse_enemy_object) != 0)
    result = -1;

  free(buffer);

  if (result != 0)
  {
    worldSpriteCount = 0;
    return -1;
  }

  return 0;
}

Sprite *entities_createWorldSprites(void)
{
  if (worldInitialized)
    return worldSprites;

  worldSpriteCount = 0;

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
}
