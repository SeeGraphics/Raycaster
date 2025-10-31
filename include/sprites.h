#ifndef SPRITES_H
#define SPRITES_H

#include "animation.h"
#include "types.h"

#define NUM_SPRITES 256

// Texture index macros (kept for legacy sprite setup convenience)
#define TEX_PILLAR 13
#define TEX_BARREL 14
#define TEX_GREENLIGHT 15
#define TEX_MONEY 16

typedef enum
{
  SPRITE_DECORATION = 0,
  SPRITE_PICKUP,
  SPRITE_ENEMY,
  SPRITE_DECAL,
} SpriteKind;

typedef enum
{
  SPRITE_ACTION_NONE = 0,
  SPRITE_ACTION_OPEN_DOOR = 1,
} SpriteActionType;

#define SPRITE_STATE_ACTIVATED (1 << 0)

typedef enum
{
  SPRITE_VISUAL_TEXTURE = 0,
  SPRITE_VISUAL_ANIMATION,
} SpriteVisualType;

typedef struct
{
  SpriteVisualType type;
  union
  {
    struct
    {
      i32 textureId;
    } texture;
    struct
    {
      Animation *animation;
    } anim;
  };
} SpriteAppearance;

typedef struct Sprite
{
  f64 x;
  f64 y;
  f32 scale;
  SpriteKind kind;
  SpriteAppearance appearance;
  i32 active;
  i32 health;
  i32 auxTextureId;
  i32 targetX;
  i32 targetY;
  i32 actionType;
  i32 stateFlags;
} Sprite;

// forward declaration
struct Engine;

SpriteAppearance spriteAppearanceFromTexture(i32 textureId);
SpriteAppearance spriteAppearanceFromAnimation(Animation *animation);

// calculations
void perform_spritecasting(struct Engine *engine);
void sortSprites(i32 *order, f64 *dist, i32 amount);

#endif
