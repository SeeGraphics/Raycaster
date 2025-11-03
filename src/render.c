#include "engine.h"
#include "raycast.h"
#include "weapons.h"
#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct
{
  u32 *pixels;
  int width;
  int height;
} HudTexture;

static HudTexture g_hudDigits[10];
static int g_hudAssetsLoaded = 0;
static const double TWO_PI = 6.28318530717958647692;

static inline float clamp01f(float value)
{
  if (value < 0.0f)
    return 0.0f;
  if (value > 1.0f)
    return 1.0f;
  return value;
}

static HudTexture hud_loadTexture(const char *path)
{
  HudTexture tex = {0};
  SDL_Surface *surface = IMG_Load(path);
  if (!surface)
  {
    fprintf(stderr, "\033[31m[HUD] Failed to load %s: %s\033[0m\n", path, IMG_GetError());
    return tex;
  }

  SDL_Surface *converted =
      SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
  SDL_FreeSurface(surface);

  if (!converted)
  {
    fprintf(stderr, "\033[31m[HUD] Failed to convert %s: %s\033[0m\n", path, SDL_GetError());
    return tex;
  }

  tex.width = converted->w;
  tex.height = converted->h;
  tex.pixels = (u32 *)malloc((size_t)tex.width * (size_t)tex.height * sizeof(u32));
  if (!tex.pixels)
  {
    fprintf(stderr, "\033[31m[HUD] Failed to allocate buffer for %s\033[0m\n", path);
    SDL_FreeSurface(converted);
    tex.width = tex.height = 0;
    return tex;
  }

  u32 *srcPixels = (u32 *)converted->pixels;
  int pitch = converted->pitch / 4;
  for (int y = 0; y < tex.height; ++y)
    for (int x = 0; x < tex.width; ++x)
      tex.pixels[y * tex.width + x] = srcPixels[y * pitch + x];

  SDL_FreeSurface(converted);
  return tex;
}

static inline int hud_scaledWidth(const HudTexture *tex, float scale)
{
  return (tex && tex->width > 0) ? (int)((float)tex->width * scale) : 0;
}

static inline int hud_scaledHeight(const HudTexture *tex, float scale)
{
  return (tex && tex->height > 0) ? (int)((float)tex->height * scale) : 0;
}

static void hud_blitTexture(u32 *buffer, const HudTexture *tex, int x, int y, float scale)
{
  if (!buffer || !tex || !tex->pixels || tex->width <= 0 || tex->height <= 0)
    return;

  int width = hud_scaledWidth(tex, scale);
  int height = hud_scaledHeight(tex, scale);
  if (width <= 0 || height <= 0)
    return;

  float invScaleX = (float)tex->width / (float)width;
  float invScaleY = (float)tex->height / (float)height;

  for (int dy = 0; dy < height; ++dy)
  {
    int screenY = y + dy;
    if (screenY < 0 || screenY >= RENDER_HEIGHT)
      continue;

    int srcY = (int)((float)dy * invScaleY);
    if (srcY < 0)
      srcY = 0;
    if (srcY >= tex->height)
      srcY = tex->height - 1;

    for (int dx = 0; dx < width; ++dx)
    {
      int screenX = x + dx;
      if (screenX < 0 || screenX >= RENDER_WIDTH)
        continue;

      int srcX = (int)((float)dx * invScaleX);
      if (srcX < 0)
        srcX = 0;
      if (srcX >= tex->width)
        srcX = tex->width - 1;

      u32 color = tex->pixels[srcY * tex->width + srcX];
      if ((color & 0xFF000000u) == 0)
        continue;

      buffer[screenY * RENDER_WIDTH + screenX] = color;
    }
  }
}

static void weapon_computeBob(const Player *player, float *offsetX, float *offsetY,
                              float *intensityOut)
{
  if (!player)
    return;

  double speed =
      sqrt(player->velX * player->velX + player->velY * player->velY);
  double normalized =
      (player->moveSpeed > 0.0) ? fmin(speed / player->moveSpeed, 1.0) : 0.0;

  static float smoothed = 0.0f;
  const float lerpFactor = 0.18f;
  float target = (float)normalized;
  smoothed += (target - smoothed) * lerpFactor;

  if (intensityOut)
    *intensityOut = smoothed;

  if (offsetX)
  {
    double swayPhase = player->bobTime * TWO_PI * 0.95;
    *offsetX = (float)(sin(swayPhase) * 14.0 * smoothed);
  }
  if (offsetY)
  {
    double bobPhase = player->bobTime * TWO_PI * 1.6;
    *offsetY = (float)(sin(bobPhase) * 9.0 * smoothed);
  }
}

static void drawWeaponLayer(Engine *engine)
{
  float offsetX = 0.0f;
  float offsetY = 0.0f;
  float intensity = 0.0f;
  weapon_computeBob(&engine->player, &offsetX, &offsetY, &intensity);

  const float baseScale = 1.5f + intensity * 0.08f;
  const float center = (float)RENDER_WIDTH * 0.5f + offsetX;
  const float baseY = (float)RENDER_HEIGHT - 150.0f + offsetY;
  const float defaultLeft = center - 75.0f;
  const float minigunLeft = center - 95.0f;

  switch (engine->player.selectedGun)
  {
  case SHOTGUN:
    blitAnimation(engine->game.Rbuffer, &animations.shotgun_shoot, RENDER_WIDTH,
                  RENDER_HEIGHT, defaultLeft, baseY, baseScale);
    break;
  case ROCKET:
    blitAnimation(engine->game.Rbuffer, &animations.rocket_shoot, RENDER_WIDTH,
                  RENDER_HEIGHT, defaultLeft, baseY, baseScale);
    break;
  case PISTOL:
    blitAnimation(engine->game.Rbuffer, &animations.pistol_shoot, RENDER_WIDTH,
                  RENDER_HEIGHT, defaultLeft, baseY, baseScale);
    break;
  case SINGLE:
    blitAnimation(engine->game.Rbuffer, &animations.single_shoot, RENDER_WIDTH,
                  RENDER_HEIGHT, defaultLeft, baseY, baseScale);
    break;
  case MINIGUN:
    if (animations.minigun_shoot.playing)
    {
      blitAnimation(engine->game.Rbuffer, &animations.minigun_shoot,
                    RENDER_WIDTH, RENDER_HEIGHT, minigunLeft, baseY, baseScale);
    }
    else
    {
      blitAnimation(engine->game.Rbuffer, &animations.minigun_idle,
                    RENDER_WIDTH, RENDER_HEIGHT, minigunLeft, baseY, baseScale);
    }
    break;
  default:
    break;
  }
}

static void renderDamageOverlay(Engine *engine)
{
  if (!engine || !engine->game.Rbuffer)
    return;

  float alpha = 0.0f;
  if (engine->player.damageFlashTimer > 0.0)
  {
    float normalized = clamp01f((float)(engine->player.damageFlashTimer /
                                        PLAYER_DAMAGE_FLASH_DURATION));
    alpha = fmaxf(alpha, normalized * 0.55f);
  }
  if (engine->player.health <= 0)
    alpha = fmaxf(alpha, 0.75f);

  if (alpha <= 0.001f)
    return;

  const float invAlpha = 1.0f - alpha;
  const Uint8 srcR = 255;
  const Uint8 srcG = 0;
  const Uint8 srcB = 0;

  u32 *buffer = engine->game.Rbuffer;
  const int pixelCount = RENDER_WIDTH * RENDER_HEIGHT;
  for (int i = 0; i < pixelCount; ++i)
  {
    u32 dst = buffer[i];
    Uint8 dstR = (Uint8)((dst >> 16) & 0xFFu);
    Uint8 dstG = (Uint8)((dst >> 8) & 0xFFu);
    Uint8 dstB = (Uint8)(dst & 0xFFu);

    Uint8 outR = (Uint8)(srcR * alpha + dstR * invAlpha);
    Uint8 outG = (Uint8)(srcG * alpha + dstG * invAlpha);
    Uint8 outB = (Uint8)(srcB * alpha + dstB * invAlpha);

    buffer[i] = (0xFFu << 24) | ((u32)outR << 16) | ((u32)outG << 8) | outB;
  }
}

static void renderDeathMessage(Engine *engine)
{
  if (!engine)
    return;
  if (engine->player.health > 0)
    return;

  TTF_Font *font = engine->font.ui;
  if (!font)
    return;

  const char *message = "You have Died! Press R to respawn.";
  int textW = 0;
  int textH = 0;
  if (TTF_SizeText(font, message, &textW, &textH) != 0)
  {
    textW = 0;
    textH = 0;
  }

  int x = (RENDER_WIDTH - textW) / 2;
  int y = (RENDER_HEIGHT - textH) / 2;
  SDL_Color color = {255, 255, 255, 255};
  renderText(engine->game.Rbuffer, font, message, x, y, color);
}

static int hud_drawDigits(u32 *buffer, const char *digits, int centerX, int y, float scale)
{
  if (!digits)
    return 0;
  int length = (int)strlen(digits);
  if (length == 0)
    return 0;

  int totalWidth = 0;
  int widths[32];
  if (length > 32)
    length = 32;

  for (int i = 0; i < length; ++i)
  {
    int idx = digits[i] - '0';
    if (idx < 0 || idx > 9 || !g_hudDigits[idx].pixels)
    {
      widths[i] = (int)(scale * 10.0f);
    }
    else
    {
      widths[i] = hud_scaledWidth(&g_hudDigits[idx], scale);
    }
    totalWidth += widths[i];
    if (i + 1 < length)
      totalWidth += (int)(scale * 6.0f);
  }

  int startX = centerX - totalWidth / 2;
  int cursor = startX;
  for (int i = 0; i < length; ++i)
  {
    int idx = digits[i] - '0';
    if (idx >= 0 && idx <= 9 && g_hudDigits[idx].pixels)
      hud_blitTexture(buffer, &g_hudDigits[idx], cursor, y, scale);
    cursor += widths[i] + (int)(scale * 6.0f);
  }

  return totalWidth;
}

static void hud_renderNumber(u32 *buffer, int value, int centerX, int y, float scale)
{
  char temp[32];
  if (value < 0)
    snprintf(temp, sizeof(temp), "0");
  else
    snprintf(temp, sizeof(temp), "%d", value);
  hud_drawDigits(buffer, temp, centerX, y, scale);
}

static void hud_renderString(u32 *buffer, const char *text, int centerX, int y,
                             float scale)
{
  hud_drawDigits(buffer, text, centerX, y, scale);
}

static void hud_initAssets(void)
{
  if (g_hudAssetsLoaded)
    return;

  char path[256];
  for (int i = 0; i < 10; ++i)
  {
    snprintf(path, sizeof(path), "assets/textures/HUD/numbers/%d.png", i);
    g_hudDigits[i] = hud_loadTexture(path);
  }

  g_hudAssetsLoaded = 1;
}

void drawDebugHUD(Engine *engine) {
  // FPS counter
  renderInt(engine->game.Rbuffer, engine->font.debug, "FPS:", engine->fps, 10,
            0, RGB_Yellow);
  // Coordinates
  renderf32Pair(engine->game.Rbuffer, engine->font.debug,
                  "POS:", engine->player.posX, engine->player.posY, 10, 15,
                  RGB_Yellow);
  // direction
  renderf32Pair(engine->game.Rbuffer, engine->font.debug,
                  "DIR:", engine->player.dirX, engine->player.dirY, 10, 30,
                  RGB_Yellow);
  // pitch
  renderf32(engine->game.Rbuffer, engine->font.debug,
              "PITCH:", engine->player.pitch, 10, 45, RGB_Yellow);
  // plane
  renderf32Pair(engine->game.Rbuffer, engine->font.debug,
                  "PLANE:", engine->player.planeX, engine->player.planeY, 10,
                  60, RGB_Yellow);
}

void drawGameHUD(Engine *engine) {
  hud_initAssets();

  u32 *buffer = engine->game.Rbuffer;
  const float digitScale = 0.45f;
  const int digitsY = RENDER_HEIGHT - 70;
  const int healthCenterX = RENDER_WIDTH / 2 - 200;
  const int ammoCenterX = RENDER_WIDTH / 2 + 200;

  hud_renderNumber(buffer, engine->player.health, healthCenterX, digitsY,
                   digitScale);

  int ammunition = weaponProperties[engine->player.selectedGun].ammunition;
  if (ammunition < 0)
    hud_renderString(buffer, "00", ammoCenterX, digitsY, digitScale);
  else
    hud_renderNumber(buffer, ammunition, ammoCenterX, digitsY, digitScale);
}

void drawDebug(Engine *engine) {
  /* 1. Clear Buffer */
  clearBuffer(&engine->game);

  /* 2. Draw Game */
  perform_floorcasting(engine);
  perform_raycasting(engine);

  if (!engine->game.buffer) {
    fprintf(stderr, "[ERROR] game.buffer is NULL!\n");
  }
  if (!engine->game.Rbuffer) {
    fprintf(stderr, "[ERROR] game.Rbuffer is NULL!\n");
  }
  if (!engine->game.Zbuffer) {
    fprintf(stderr, "[ERROR] game.Zbuffer is NULL!\n");
  }
  perform_spritecasting(engine);
  drawWeaponLayer(engine);
  drawDebugHUD(engine);
  drawGameHUD(engine);
  renderDamageOverlay(engine);
  renderDeathMessage(engine);
  drawBuffer(&engine->game);
}

void drawGame(Engine *engine) {

  /* 1. Clear Buffer */
  clearBuffer(&engine->game);

  /* 2. Draw Game */
  perform_floorcasting(engine);
  perform_raycasting(engine);

  if (!engine->game.buffer) {
    fprintf(stderr, "[ERROR] game.buffer is NULL!\n");
  }
  if (!engine->game.Rbuffer) {
    fprintf(stderr, "[ERROR] game.Rbuffer is NULL!\n");
  }
  if (!engine->game.Zbuffer) {
    fprintf(stderr, "[ERROR] game.Zbuffer is NULL!\n");
  }
  perform_spritecasting(engine);
  drawWeaponLayer(engine);
  drawGameHUD(engine);
  renderDamageOverlay(engine);
  renderDeathMessage(engine);
  drawBuffer(&engine->game);
}

void drawScene(Engine *engine) {
  switch (engine->mode) {
  case GAME:
    drawGame(engine);
    break;
  case DEBUG:
    drawDebug(engine);
    break;
  }
}
