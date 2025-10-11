#ifndef FONT_H
#define FONT_H

#include "SDL_ttf.h"

typedef struct {
  TTF_Font *title;
  TTF_Font *debug;
  TTF_Font *ui;
} Font;

void renderText(SDL_Renderer *renderer, TTF_Font *font, const char *message,
                int x, int y, SDL_Color color);

#endif
