#include "sprites.h"
#include "graphics.h"
#include "player.h"
#include "texture.h"

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

void perform_spritecasting(Game *game, Sprite *sprite, TextureManager *tm,
                           Player *player) {
  int spriteOrder[NUM_SPRITES];
  double spriteDistance[NUM_SPRITES];

  // sort sprites from far to close
  for (int i = 0; i < NUM_SPRITES; i++) {
    spriteOrder[i] = i;
    spriteDistance[i] =
        ((player->posX - sprite[i].x) * (player->posX - sprite[i].x) +
         (player->posY - sprite[i].y) * (player->posY - sprite[i].y));
  }

  sortSprites(spriteOrder, spriteDistance, NUM_SPRITES);
  // after sorting the sprites, do the projection and draw them
  for (int i = 0; i < NUM_SPRITES; i++) {
    // translate sprite position to relative to camera
    double spriteX = sprite[spriteOrder[i]].x - player->posX;
    double spriteY = sprite[spriteOrder[i]].y - player->posY;

    double invDet =
        1.0 /
        (player->planeX * player->dirY -
         player->dirX *
             player->planeY); // required for correct matrix multiplication

    double transformX =
        invDet * (player->dirY * spriteX - player->dirX * spriteY);
    double transformY =
        invDet *
        (-player->planeY * spriteX +
         player->planeX * spriteY); // this is actually the depth inside the
                                    // screen, that what Z is in 3D

    int spriteScreenX =
        (int)((game->window_width / 2) * (1 + transformX / transformY));

    // calculate height of the sprite on screen
    int spriteHeight = fabs((int)game->window_height /
                            (transformY)); // using 'transformY' instead of the
                                           // real distance prevents fisheye
    // calculate lowest and highest pixel to fill in current stripe
    int drawStartY =
        -spriteHeight / 2 + game->window_height / 2 + player->pitch;
    if (drawStartY < 0)
      drawStartY = 0;
    int drawEndY = spriteHeight / 2 + game->window_height / 2 + player->pitch;
    if (drawEndY >= game->window_height)
      drawEndY = game->window_height - 1;

    // calculate width of the sprite
    int spriteWidth = fabs((int)game->window_height / (transformY));
    int drawStartX = -spriteWidth / 2 + spriteScreenX;
    if (drawStartX < 0)
      drawStartX = 0;
    int drawEndX = spriteWidth / 2 + spriteScreenX;
    if (drawEndX >= game->window_width)
      drawEndX = game->window_width - 1;

    // loop through every vertical stripe of the sprite on screen
    for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
      int texX = (int)256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) *
                 TEXT_WIDTH / spriteWidth / 256;
      // the conditions in the if are:
      // 1) it's in front of camera plane so you don't see things behind you
      // 2) it's on the screen (left)
      // 3) it's on the screen (right)
      // 4) ZBuffer, with perpendicular distance
      if (transformY > 0 && stripe >= 0 && stripe < game->window_width &&
          transformY < game->Zbuffer[stripe])

        for (int y = drawStartY; y < drawEndY;
             y++) // for every pixel of the current stripe
        {
          int d = (y - player->pitch) * 256 - game->window_height * 128 +
                  spriteHeight * 128; // 256 and 128 factors to avoid floats
          int texY = ((d * TEXT_HEIGHT) / spriteHeight) / 256;

          if (sprite[spriteOrder[i]].texture < 0 ||
              sprite[spriteOrder[i]].texture >= NUM_TEXTURES)
            continue;

          Uint32 color =
              tm->textures[sprite[spriteOrder[i]].texture]
                          [TEXT_WIDTH * texY +
                           texX]; // get current color from the texture
                                  // Only draw non-transparent pixels
          // clamp to avoid crash
          if (texX < 0)
            texX = 0;
          if (texX >= TEXT_WIDTH)
            texX = TEXT_WIDTH - 1;
          if (texY < 0)
            texY = 0;
          if (texY >= TEXT_HEIGHT)
            texY = TEXT_HEIGHT - 1;

          if ((color & 0x00FFFFFF) != 0)
            game->buffer[y * game->window_width + stripe] = color;
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
