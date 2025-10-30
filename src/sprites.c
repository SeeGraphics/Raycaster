#include "sprites.h"
#include "engine.h"
#include <math.h>

typedef struct
{
  const u32 *pixels;
  i32 width;
  i32 height;
} SpriteFrame;

SpriteAppearance spriteAppearanceFromTexture(i32 textureId)
{
  SpriteAppearance appearance;
  appearance.type = SPRITE_VISUAL_TEXTURE;
  appearance.texture.textureId = textureId;
  return appearance;
}

SpriteAppearance spriteAppearanceFromAnimation(Animation *animation)
{
  SpriteAppearance appearance;
  appearance.type = SPRITE_VISUAL_ANIMATION;
  appearance.anim.animation = animation;
  return appearance;
}

static int sprite_acquireFrame(const Sprite *sprite, const Engine *engine,
                               SpriteFrame *outFrame)
{
  if (!sprite || !engine || !outFrame || !sprite->active)
    return 0;

  if (sprite->appearance.type == SPRITE_VISUAL_TEXTURE)
  {
    i32 texIndex = sprite->appearance.texture.textureId;
    if (texIndex < 0 || texIndex >= NUM_TEXTURES)
      return 0;

    const u32 *pixels = engine->textures.textures[texIndex];
    if (!pixels)
      return 0;

    outFrame->pixels = pixels;
    outFrame->width = TEXT_WIDTH;
    outFrame->height = TEXT_HEIGHT;
    return 1;
  }

  if (sprite->appearance.type == SPRITE_VISUAL_ANIMATION)
  {
    Animation *animation = sprite->appearance.anim.animation;
    if (!animation || animation->frameCount <= 0 || !animation->frames)
      return 0;

    if (animation->currentFrame < 0 ||
        animation->currentFrame >= animation->frameCount)
      return 0;

    Frame *frame = &animation->frames[animation->currentFrame];
    if (!frame->pixels || frame->width <= 0 || frame->height <= 0)
      return 0;

    outFrame->pixels = frame->pixels;
    outFrame->width = frame->width;
    outFrame->height = frame->height;
    return 1;
  }

  return 0;
}

static int sprite_isTransparent(const Sprite *sprite, u32 color)
{
  if (sprite->appearance.type == SPRITE_VISUAL_ANIMATION)
  {
    return (color & 0xFF000000u) == 0;
  }
  return (color & 0x00FFFFFFu) == 0;
}

void perform_spritecasting(Engine *engine)
{
  Sprite *sprites = engine->sprites;
  i32 spriteOrder[NUM_SPRITES];
  f64 spriteDistance[NUM_SPRITES];

  for (i32 i = 0; i < NUM_SPRITES; ++i)
  {
    spriteOrder[i] = i;
    f64 dx = engine->player.posX - sprites[i].x;
    f64 dy = engine->player.posY - sprites[i].y;
    spriteDistance[i] = dx * dx + dy * dy;
  }

  sortSprites(spriteOrder, spriteDistance, NUM_SPRITES);

  for (i32 i = 0; i < NUM_SPRITES; ++i)
  {
    Sprite *sprite = &sprites[spriteOrder[i]];
    if (!sprite->active)
      continue;

    f64 spriteX = sprite->x - engine->player.posX;
    f64 spriteY = sprite->y - engine->player.posY;

    f64 det =
        engine->player.planeX * engine->player.dirY -
        engine->player.dirX * engine->player
                                  .planeY; // required for correct matrix
                                           // multiplication
    if (fabs(det) < 1e-8)
      continue;

    f64 invDet = 1.0 / det;

    f64 transformX =
        invDet * (engine->player.dirY * spriteX -
                  engine->player.dirX * spriteY);
    f64 transformY =
        invDet * (-engine->player.planeY * spriteX +
                  engine->player.planeX *
                      spriteY); // depth inside screen
    if (transformY <= 0.0)
      continue;

    i32 spriteScreenX = (i32)((f64)RENDER_WIDTH / 2.0 *
                              (1.0 + transformX / transformY));

    SpriteFrame frame;
    if (!sprite_acquireFrame(sprite, engine, &frame))
      continue;

    if (frame.width <= 0 || frame.height <= 0)
      continue;

    f64 projectedHeight = ((f64)RENDER_HEIGHT / transformY) * sprite->scale;
    i32 spriteHeight = (i32)fabs(projectedHeight);
    if (spriteHeight <= 0)
      continue;

    f64 aspectRatio = (f64)frame.width / (f64)frame.height;
    if (frame.height == 0)
      aspectRatio = 1.0;
    i32 spriteWidth = (i32)fabs(spriteHeight * aspectRatio);
    if (spriteWidth <= 0)
      continue;

    i32 spriteTop =
        -spriteHeight / 2 + RENDER_HEIGHT / 2 + (i32)engine->player.pitch;
    i32 spriteBottom =
        spriteHeight / 2 + RENDER_HEIGHT / 2 + (i32)engine->player.pitch;
    i32 spriteLeft = -spriteWidth / 2 + spriteScreenX;
    i32 spriteRight = spriteWidth / 2 + spriteScreenX;

    i32 drawStartY = spriteTop < 0 ? 0 : spriteTop;
    i32 drawEndY =
        spriteBottom >= RENDER_HEIGHT ? RENDER_HEIGHT - 1 : spriteBottom;
    i32 drawStartX = spriteLeft < 0 ? 0 : spriteLeft;
    i32 drawEndX = spriteRight >= RENDER_WIDTH ? RENDER_WIDTH - 1 : spriteRight;

    if (drawStartX > drawEndX || drawStartY > drawEndY)
      continue;

    f64 invSpriteWidth = 1.0 / (f64)spriteWidth;
    f64 invSpriteHeight = 1.0 / (f64)spriteHeight;

    for (i32 stripe = drawStartX; stripe <= drawEndX; ++stripe)
    {
      if (stripe < 0 || stripe >= RENDER_WIDTH)
        continue;

      if (transformY >= engine->game.Zbuffer[stripe])
        continue;

      f64 relativeX = (stripe - spriteLeft) * invSpriteWidth;
      if (relativeX < 0.0 || relativeX > 1.0)
        continue;

      i32 texX = (i32)(relativeX * frame.width);
      if (texX < 0)
        texX = 0;
      if (texX >= frame.width)
        texX = frame.width - 1;

      for (i32 y = drawStartY; y <= drawEndY; ++y)
      {
        if (y < 0 || y >= RENDER_HEIGHT)
          continue;

        f64 relativeY = (y - spriteTop) * invSpriteHeight;
        if (relativeY < 0.0 || relativeY > 1.0)
          continue;

        i32 texY = (i32)(relativeY * frame.height);
        if (texY < 0)
          texY = 0;
        if (texY >= frame.height)
          texY = frame.height - 1;

        u32 color = frame.pixels[texY * frame.width + texX];
        if (sprite_isTransparent(sprite, color))
          continue;

        engine->game.Rbuffer[y * RENDER_WIDTH + stripe] = color;
      }
    }
  }
}

void sortSprites(i32 *order, f64 *dist, i32 amount)
{
  for (i32 i = 1; i < amount; ++i)
  {
    i32 tempOrder = order[i];
    f64 tempDist = dist[i];
    i32 j = i - 1;

    while (j >= 0 && dist[j] < tempDist)
    {
      order[j + 1] = order[j];
      dist[j + 1] = dist[j];
      --j;
    }

    order[j + 1] = tempOrder;
    dist[j + 1] = tempDist;
  }
}
