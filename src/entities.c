#include "entities.h"
#include "animation.h"

typedef struct
{
  f32 x;
  f32 y;
  f32 scale;
  i32 textureId;
} DecorationSpawn;

typedef struct
{
  f32 x;
  f32 y;
  f32 scale;
  i32 textureId;
} PickupSpawn;

typedef struct
{
  f32 x;
  f32 y;
  f32 scale;
  i32 health;
  Animation *animation;
} EnemySpawn;

static Sprite worldSprites[NUM_SPRITES];
static i32 worldSpriteCount = 0;
static int worldInitialized = 0;

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

static void spawn_decorations(void)
{
  static const DecorationSpawn greenLights[] = {
      {4.5f, 3.5f, 1.0f, TEX_GREENLIGHT},  {8.5f, 4.5f, 1.0f, TEX_GREENLIGHT},
      {15.5f, 4.5f, 1.0f, TEX_GREENLIGHT}, {18.5f, 4.5f, 1.0f, TEX_GREENLIGHT},
      {7.5f, 8.5f, 1.0f, TEX_GREENLIGHT},  {7.5f, 16.5f, 1.0f, TEX_GREENLIGHT},
      {16.5f, 8.5f, 1.0f, TEX_GREENLIGHT}, {16.5f, 16.5f, 1.0f, TEX_GREENLIGHT},
  };

  static const DecorationSpawn pillars[] = {
      {6.5f, 5.5f, 1.0f, TEX_PILLAR}, {6.5f, 6.5f, 1.0f, TEX_PILLAR},
      {17.5f, 5.5f, 1.0f, TEX_PILLAR}, {17.5f, 6.5f, 1.0f, TEX_PILLAR},
      {9.5f, 9.5f, 1.0f, TEX_PILLAR}, {14.5f, 9.5f, 1.0f, TEX_PILLAR},
  };

  static const DecorationSpawn barrels[] = {
      {4.5f, 18.5f, 1.0f, TEX_BARREL},  {6.5f, 18.5f, 1.0f, TEX_BARREL},
      {9.5f, 18.5f, 1.0f, TEX_BARREL},  {14.5f, 16.5f, 1.0f, TEX_BARREL},
      {17.5f, 16.5f, 1.0f, TEX_BARREL}, {12.5f, 20.5f, 1.0f, TEX_BARREL},
  };

  const DecorationSpawn *sets[] = {greenLights, pillars, barrels};
  const i32 counts[] = {(i32)(sizeof(greenLights) / sizeof(greenLights[0])),
                        (i32)(sizeof(pillars) / sizeof(pillars[0])),
                        (i32)(sizeof(barrels) / sizeof(barrels[0]))};

  for (i32 set = 0; set < (i32)(sizeof(sets) / sizeof(sets[0])); ++set)
  {
    const DecorationSpawn *spawns = sets[set];
    for (i32 i = 0; i < counts[set]; ++i)
    {
      const DecorationSpawn *spawn = &spawns[i];
      Sprite sprite = sprite_make(spawn->x, spawn->y, SPRITE_DECORATION,
                                  spriteAppearanceFromTexture(spawn->textureId),
                                  spawn->scale, 0);
      entities_pushSprite(sprite);
    }
  }
}

static void spawn_pickups(void)
{
  static const PickupSpawn pickups[] = {
      {12.5f, 7.5f, 0.9f, TEX_MONEY},
      {11.5f, 15.5f, 0.9f, TEX_MONEY},
  };

  for (i32 i = 0; i < (i32)(sizeof(pickups) / sizeof(pickups[0])); ++i)
  {
    const PickupSpawn *spawn = &pickups[i];
    Sprite sprite = sprite_make(spawn->x, spawn->y, SPRITE_PICKUP,
                                spriteAppearanceFromTexture(spawn->textureId),
                                spawn->scale, 0);
    entities_pushSprite(sprite);
  }
}

static void spawn_enemies(void)
{
  static EnemySpawn enemies[] = {
      {15.5f, 13.5f, 1.35f, 200, &animations.demon_walk},
  };

  for (i32 i = 0; i < (i32)(sizeof(enemies) / sizeof(enemies[0])); ++i)
  {
    EnemySpawn *spawn = &enemies[i];
    Sprite sprite =
        sprite_make(spawn->x, spawn->y, SPRITE_ENEMY,
                    spriteAppearanceFromAnimation(spawn->animation),
                    spawn->scale, spawn->health);
    entities_pushSprite(sprite);
  }
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

Sprite *entities_createWorldSprites(void)
{
  if (worldInitialized)
    return worldSprites;

  worldSpriteCount = 0;

  spawn_decorations();
  spawn_pickups();
  spawn_enemies();
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
