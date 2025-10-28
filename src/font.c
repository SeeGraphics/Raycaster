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
      buffer[dstIndex] = color;
    }
  }

  SDL_FreeSurface(surface);
  SDL_FreeSurface(converted);
}

void renderFloatPair(u32 *Rbuffer, TTF_Font *font, const char *label, double x,
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

void renderFloat(u32 *Rbuffer, TTF_Font *font, const char *label, double value,
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
