#include "font.h"

Font font_init() {
  Font f = {
      TTF_OpenFont("assets/font/Doom.ttf", 120), // title
      TTF_OpenFont("assets/font/Doom.ttf", 40),  // debug
      TTF_OpenFont("assets/font/Doom.ttf", 90),  // UI
  };

  return f;
}

void renderText(SDL_Renderer *renderer, TTF_Font *font, const char *message,
                int x, int y, SDL_Color color) {
  // create surface, texture, pos/size
  SDL_Surface *surface = TTF_RenderText_Blended(font, message, color);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_Rect rect = {x, y, surface->w, surface->h};

  // draw
  SDL_RenderCopy(renderer, texture, NULL, &rect);

  // cleanup
  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);
}

void renderFloatPair(SDL_Renderer *renderer, TTF_Font *font, const char *label,
                     double x, double y, int xpos, int ypos, SDL_Color color) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%s: %.2f %.2f", label, x, y);
  renderText(renderer, font, buffer, xpos, ypos, color);
}

void renderInt(SDL_Renderer *renderer, TTF_Font *font, const char *label,
               int value, int x, int y, SDL_Color color) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%s: %d", label, value);
  renderText(renderer, font, buffer, x, y, color);
}

void renderFloat(SDL_Renderer *renderer, TTF_Font *font, const char *label,
                 double value, int x, int y, SDL_Color color) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%s: %.2f", label, value);
  renderText(renderer, font, buffer, x, y, color);
}
