#include "font.h"

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
