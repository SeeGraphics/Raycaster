#ifndef TEXTURE_H
#define TEXTURE_H

#include <stdint.h>

// texture size
#define TEXT_HEIGHT 64
#define TEXT_WIDTH 64

// texture count
#define NUM_WALL_TEXTURES 8
#define NUM_DECOR_TEXTURES 3
#define NUM_ENTITY_TEXTURES 1
#define NUM_TEXTURES                                                           \
  (NUM_WALL_TEXTURES + NUM_DECOR_TEXTURES + NUM_ENTITY_TEXTURES)

typedef struct {
  uint32_t *textures[NUM_TEXTURES];
} TextureManager;

typedef struct {
  const char *path;
  const char *name;
} TextureInfo;

/* TEXTURE ARRAYS */
static const TextureInfo wallTextures[NUM_WALL_TEXTURES] = {
    {"assets/textures/sides/eagle.png", "Eagle"},
    {"assets/textures/sides/redbrick.png", "RedBrick"},
    {"assets/textures/sides/purplestone.png", "PurpleStone"},
    {"assets/textures/sides/greystone.png", "GreyStone"},
    {"assets/textures/sides/bluestone.png", "BlueStone"},
    {"assets/textures/sides/mossy.png", "Mossy"},
    {"assets/textures/sides/wood.png", "Wood"},
    {"assets/textures/sides/colorstone.png", "ColorStone"}};

static const TextureInfo decorTextures[NUM_DECOR_TEXTURES] = {
    {"assets/textures/decorations/pillar.png", "Pillar"},
    {"assets/textures/decorations/barrel.png", "Barrel"},
    {"assets/textures/decorations/greenlight.png", "GreenLight"}};

static const TextureInfo entityTextures[NUM_ENTITY_TEXTURES] = {
    {"assets/textures/entities/money.png", "Money"}};

// init
TextureManager createTextures();
int textures_load(TextureManager *tm);

// loading
void loadImage(uint32_t *texture, int width, int height, const char *filename);
void loadArrays(TextureManager *tm, int texWidth, int texHeight);
int getTextureIndexByName(const char *name);

#endif
