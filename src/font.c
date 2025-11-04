#include "font.h"
#include "graphics.h"
#include "types.h"

Font font_init() {
  Font f = {
      TTF_OpenFont("assets/font/Doom.ttf", FONTSIZE_TITLE), // title
      TTF_OpenFont("assets/font/Doom.ttf", FONTSIZE_DEBUG), // debug
      TTF_OpenFont("assets/font/Doom.ttf", FONTSIZE_UI),    // UI
  };

  return f;
}

void renderText(u32 *buffer, TTF_Font *font, const char *message, int posx,
                int posy, SDL_Color color) {
  // create surface, texture, pos/size
  SDL_Surface *surface = TTF_RenderText_Blended(font, message, color);
  if (!surface)
    fprintf(stderr, "\033[31m[ERROR] Failed to create Surface: %s\033[0m\n",
            SDL_GetError());

  SDL_Surface *converted =
      SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
  if (!converted)
    fprintf(stderr, "\033[31m[ERROR] Failed to create Surface: %s\033[0m\n",
            SDL_GetError());

  int width = converted->w;
  int height = converted->h;

  u32 *image = (u32 *)converted->pixels;
  if (!image)
    fprintf(stderr, "\033[31m[ERROR] Failed to allocate buffer:  %s\033[0m\n",
            SDL_GetError());

  float overallAlpha = color.a / 255.0f;
  if (overallAlpha <= 0.0f)
  {
    SDL_FreeSurface(surface);
    SDL_FreeSurface(converted);
    return;
  }

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {

      int screenY = y + posy;
      int screenX = x + posx;

      if (screenX < 0 || screenX >= RENDER_WIDTH || screenY < 0 ||
          screenY >= RENDER_HEIGHT) {
        continue;
      }

      int image_index = y * (converted->pitch / 4) + x;
      u32 color = image[image_index];

      if ((color & 0xFF000000) == 0)
        continue;

      int dstIndex = screenY * RENDER_WIDTH + screenX;
      if (overallAlpha >= 0.999f)
      {
        buffer[dstIndex] = color;
      }
      else
      {
        u32 dst = buffer[dstIndex];
        Uint8 srcA = (Uint8)((color >> 24) & 0xFF);
        float srcAlpha = (srcA / 255.0f) * overallAlpha;
        if (srcAlpha <= 0.0f)
          continue;
        if (srcAlpha >= 0.999f)
        {
          buffer[dstIndex] = color;
          continue;
        }

        Uint8 srcR = (Uint8)((color >> 16) & 0xFF);
        Uint8 srcG = (Uint8)((color >> 8) & 0xFF);
        Uint8 srcB = (Uint8)(color & 0xFF);
        Uint8 dstR = (Uint8)((dst >> 16) & 0xFF);
        Uint8 dstG = (Uint8)((dst >> 8) & 0xFF);
        Uint8 dstB = (Uint8)(dst & 0xFF);

        float invAlpha = 1.0f - srcAlpha;
        Uint8 outR = (Uint8)(srcR * srcAlpha + dstR * invAlpha);
        Uint8 outG = (Uint8)(srcG * srcAlpha + dstG * invAlpha);
        Uint8 outB = (Uint8)(srcB * srcAlpha + dstB * invAlpha);
        buffer[dstIndex] = (0xFFu << 24) | ((u32)outR << 16) |
                           ((u32)outG << 8) | (u32)outB;
      }
    }
  }

  SDL_FreeSurface(surface);
  SDL_FreeSurface(converted);
}

void renderf32Pair(u32 *Rbuffer, TTF_Font *font, const char *label, double x,
                     double y, int xpos, int ypos, SDL_Color color) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%s %.2f %.2f", label, x, y);
  renderText(Rbuffer, font, buffer, xpos, ypos, color);
}

void renderInt(u32 *Rbuffer, TTF_Font *font, const char *label, int value,
               int x, int y, SDL_Color color) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%s %d", label, value);
  renderText(Rbuffer, font, buffer, x, y, color);
}

void renderf32(u32 *Rbuffer, TTF_Font *font, const char *label, double value,
                 int x, int y, SDL_Color color) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%s %.2f", label, value);
  renderText(Rbuffer, font, buffer, x, y, color);
}

void renderProcent(u32 *Rbuffer, TTF_Font *font, int value, int x, int y,
                   SDL_Color color) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%d%%", value);
  renderText(Rbuffer, font, buffer, x, y, color);
}
