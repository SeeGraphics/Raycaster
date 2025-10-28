#include "texture.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>

// create Object for Engine
TextureManager createTextures() {
  TextureManager t = {NULL};
  return t;
}

int textures_load(TextureManager *tm) {
  for (int i = 0; i < NUM_TEXTURES; i++) {
    tm->textures[i] = malloc(TEXT_WIDTH * TEXT_HEIGHT * sizeof(u32));
    if (!tm->textures[i]) {
      fprintf(stderr, "\033[31mFailed to allocate texture:: %d\033[0m\n", i);
      free(tm->textures[i]);
      return 1;
    }
  }

  loadArrays(tm, TEXT_WIDTH, TEXT_HEIGHT);
  return 0;
}

void loadImage(u32 *texture, int width, int height, const char *filename) {
  SDL_Surface *surface = IMG_Load(filename);
  if (!surface) {
    fprintf(stderr, "\033[31mFailed to load %s: %s\033[0m\n", filename,
            IMG_GetError());
    return;
  }

  SDL_Surface *converted =
      SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
  SDL_FreeSurface(surface);

  if (!converted) {
    fprintf(stderr, "\033[31m[ERROR] Failed to convert %s: %s\033[0m\n",
            filename, SDL_GetError());
    return;
  }

  // Copy pixel data to texture buffer
  u32 *pixels = (u32 *)converted->pixels;
  int minW = (width < converted->w) ? width : converted->w;
  int minH = (height < converted->h) ? height : converted->h;

  for (int y = 0; y < minH; y++) {
    for (int x = 0; x < minW; x++) {
      texture[y * width + x] = pixels[y * (converted->pitch / 4) + x];
    }
  }

  SDL_FreeSurface(converted);
  printf("\033[32m[TEXTURE] Loaded texture: %s\033[0m\n", filename);
}

void loadArrays(TextureManager *tm, int texWidth, int texHeight) {
  int index = 0;

  // Load wall textures
  for (int i = 0; i < NUM_WALL_TEXTURES; i++)
    loadImage(tm->textures[index + i], texWidth, texHeight,
              wallTextures[i].path);
  index += NUM_WALL_TEXTURES;

  // Load decoration textures
  for (int i = 0; i < NUM_DECOR_TEXTURES; i++)
    loadImage(tm->textures[index + i], texWidth, texHeight,
              decorTextures[i].path);
  index += NUM_DECOR_TEXTURES;

  // Load entity textures
  for (int i = 0; i < NUM_ENTITY_TEXTURES; i++)
    loadImage(tm->textures[index + i], texWidth, texHeight,
              entityTextures[i].path);

  // Load animations?

  printf("\033[32m[TEXTURE] Textures loaded...\033[0m\n");
}

int getTextureIndexByName(const char *name) {
  // Check walls
  for (int i = 0; i < NUM_WALL_TEXTURES; i++)
    if (strcmp(wallTextures[i].name, name) == 0)
      return i;

  // Check decorations
  for (int i = 0; i < NUM_DECOR_TEXTURES; i++)
    if (strcmp(decorTextures[i].name, name) == 0)
      return NUM_WALL_TEXTURES + i;

  // Check entities
  for (int i = 0; i < NUM_ENTITY_TEXTURES; i++)
    if (strcmp(entityTextures[i].name, name) == 0)
      return NUM_WALL_TEXTURES + NUM_DECOR_TEXTURES + i;

  return -1; // not found
}
