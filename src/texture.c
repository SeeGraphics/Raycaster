#include "texture.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>

TextureManager createTextures() {
  TextureManager t = {NULL};
  return t;
}

int textures_load(TextureManager *tm) {
  for (int i = 0; i < NUM_TEXTURES; i++) {
    tm->textures[i] = malloc(TEXT_WIDTH * TEXT_HEIGHT * sizeof(uint32_t));
    if (!tm->textures[i]) {
      fprintf(stderr, "Couldn't allocate texture %d", i);
      free(tm->textures[i]);
      return 1;
    }
  }

  loadTextures(tm, TEXT_WIDTH, TEXT_HEIGHT);
  return 0;
}

void loadImage(uint32_t *texture, int width, int height, const char *filename) {
  // Load image file
  SDL_Surface *surface = IMG_Load(filename);
  if (!surface) {
    fprintf(stderr, "Failed to load %s: %s\n", filename, IMG_GetError());
    return;
  }

  // Convert to ARGB8888 format if needed
  SDL_Surface *converted =
      SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
  SDL_FreeSurface(surface);

  if (!converted) {
    fprintf(stderr, "Failed to convert %s: %s\n", filename, SDL_GetError());
    return;
  }

  // Copy pixel data to texture buffer
  uint32_t *pixels = (uint32_t *)converted->pixels;
  int minW = (width < converted->w) ? width : converted->w;
  int minH = (height < converted->h) ? height : converted->h;

  for (int y = 0; y < minH; y++) {
    for (int x = 0; x < minW; x++) {
      texture[y * width + x] = pixels[y * (converted->pitch / 4) + x];
    }
  }

  SDL_FreeSurface(converted);
  printf("Loaded texture: %s\n", filename);
}

void loadTextures(TextureManager *tm, int texWidth, int texHeight) {
  const char *texture_files[NUM_TEXTURES] = {
      // walls
      "assets/textures/sides/eagle.png",       // 0
      "assets/textures/sides/redbrick.png",    // 1
      "assets/textures/sides/purplestone.png", // 2
      "assets/textures/sides/greystone.png",   // 3
      "assets/textures/sides/bluestone.png",   // 4
      "assets/textures/sides/mossy.png",       // 5
      "assets/textures/sides/wood.png",        // 6
      "assets/textures/sides/colorstone.png",  // 7

      // decorations
      "assets/textures/decorations/pillar.png",     // 8
      "assets/textures/decorations/barrel.png",     // 9
      "assets/textures/decorations/greenlight.png", // 10

      // entities
      "assets/textures/entities/money.png", // 11
  };

  for (int i = 0; i < NUM_TEXTURES; i++) {
    loadImage(tm->textures[i], texWidth, texHeight, texture_files[i]);
  }

  printf("Textures loaded...\n");
}
