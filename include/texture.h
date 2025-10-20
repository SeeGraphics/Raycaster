#ifndef TEXTURE_H
#define TEXTURE_H

#include <stdint.h>

// texture size
#define NUM_TEXTURES 8
#define TEXT_HEIGHT 64
#define TEXT_WIDTH 64

typedef struct {
  uint32_t *textures[NUM_TEXTURES];
} TextureManager;

void loadImage(uint32_t *texture, int width, int height, const char *filename);
void loadTextures(TextureManager *tm, int texWidth, int texHeight);

#endif
