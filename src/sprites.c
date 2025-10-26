#include "sprites.h"
#include "engine.h"
#include "types.h"

Sprite *createSprite() {
  static Sprite s[NUM_SPRITES] = {
      // green lights
      {20.5, 11.5, 10},
      {18.5, 4.5, 10},
      {10.0, 4.5, 10},
      {10.0, 12.5, 10},
      {3.5, 6.5, 10},
      {3.5, 20.5, 10},
      {3.5, 14.5, 10},
      {14.5, 20.5, 10},

      // row of pillars
      {18.5, 10.5, 11},
      {18.5, 11.5, 8},
      {18.5, 12.5, 8},

      // barrels
      {21.5, 1.5, 9},
      {15.5, 1.5, 9},
      {16.0, 1.8, 9},
      {16.2, 1.2, 9},
      {3.5, 2.5, 9},
      {9.5, 15.5, 9},
      {10.0, 15.1, 9},
      {10.5, 15.8, 9},
  };

  return s;
}

void perform_spritecasting(Engine *engine) {
  int spriteOrder[NUM_SPRITES];
  double spriteDistance[NUM_SPRITES];

  // sort sprites from far to close
  for (int i = 0; i < NUM_SPRITES; i++) {
    spriteOrder[i] = i;
    spriteDistance[i] = ((engine->player.posX - engine->sprites[i].x) *
                             (engine->player.posX - engine->sprites[i].x) +
                         (engine->player.posY - engine->sprites[i].y) *
                             (engine->player.posY - engine->sprites[i].y));
  }

  sortSprites(spriteOrder, spriteDistance, NUM_SPRITES);

  // after sorting the sprites, do the projection and draw them
  for (int i = 0; i < NUM_SPRITES; i++) {
    // translate sprite position relative to camera
    double spriteX = engine->sprites[spriteOrder[i]].x - engine->player.posX;
    double spriteY = engine->sprites[spriteOrder[i]].y - engine->player.posY;

    double invDet =
        1.0 / (engine->player.planeX * engine->player.dirY -
               engine->player.dirX *
                   engine->player
                       .planeY); // required for correct matrix multiplication

    double transformX = invDet * (engine->player.dirY * spriteX -
                                  engine->player.dirX * spriteY);
    double transformY =
        invDet * (-engine->player.planeY * spriteX +
                  engine->player.planeX * spriteY); // depth inside screen

    int spriteScreenX =
        (int)((RENDER_WIDTH / 2) * (1 + transformX / transformY));

    // calculate height of the sprite on screen
    int spriteHeight = fabs((int)RENDER_HEIGHT / transformY);
    int drawStartY =
        -spriteHeight / 2 + RENDER_HEIGHT / 2 + engine->player.pitch;
    if (drawStartY < 0)
      drawStartY = 0;
    int drawEndY = spriteHeight / 2 + RENDER_HEIGHT / 2 + engine->player.pitch;
    if (drawEndY >= RENDER_HEIGHT)
      drawEndY = RENDER_HEIGHT - 1;

    // calculate width of the sprite
    int spriteWidth = fabs((int)RENDER_HEIGHT / transformY);
    int drawStartX = -spriteWidth / 2 + spriteScreenX;
    if (drawStartX < 0)
      drawStartX = 0;
    int drawEndX = spriteWidth / 2 + spriteScreenX;
    if (drawEndX >= RENDER_WIDTH)
      drawEndX = RENDER_WIDTH - 1;

    // loop through every vertical stripe of the sprite on screen
    for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
      int texX = (int)(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) *
                       TEXT_WIDTH / spriteWidth) /
                 256;

      // visibility checks
      if (transformY > 0 && stripe >= 0 && stripe < RENDER_WIDTH &&
          transformY < engine->game.Zbuffer[stripe]) {

        for (int y = drawStartY; y < drawEndY; y++) {
          int d = (y - engine->player.pitch) * 256 - RENDER_HEIGHT * 128 +
                  spriteHeight * 128;
          int texY = ((d * TEXT_HEIGHT) / spriteHeight) / 256;

          int texIndex = engine->sprites[spriteOrder[i]].texture;
          if (texIndex < 0 || texIndex >= NUM_TEXTURES)
            continue;

          // Wrap texture coordinates with bitmask: x & (64-1) == x % 64
          // Works only because texture size (64) is a power of two
          texX &= (TEXT_WIDTH - 1);
          texY &= (TEXT_HEIGHT - 1);

          Uint32 color =
              engine->textures.textures[texIndex][texY * TEXT_WIDTH + texX];

          // clamp tex coords
          if (texX < 0)
            texX = 0;
          if (texX >= TEXT_WIDTH)
            texX = TEXT_WIDTH - 1;
          if (texY < 0)
            texY = 0;
          if (texY >= TEXT_HEIGHT)
            texY = TEXT_HEIGHT - 1;

          // draw pixel if not transparent
          if ((color & 0x00FFFFFF) != 0)
            engine->game.Rbuffer[y * RENDER_WIDTH + stripe] = color;
        }
      }
    }
  }
}

void sortSprites(int *order, double *dist, int amount) {
  for (int i = 1; i < amount; i++) {
    int tempOrder = order[i];
    double tempDist = dist[i];
    int j = i - 1;

    while (j >= 0 && dist[j] < tempDist) {
      order[j + 1] = order[j];
      dist[j + 1] = dist[j];
      j--;
    }

    order[j + 1] = tempOrder;
    dist[j + 1] = tempDist;
  }
}
