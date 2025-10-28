#ifndef FONT_H
#define FONT_H

#include "SDL_ttf.h"
#include "types.h"

#define FONTSIZE_TITLE 100
#define FONTSIZE_DEBUG 20
#define FONTSIZE_UI 30

typedef struct {
  TTF_Font *title;
  TTF_Font *debug;
  TTF_Font *ui;
} Font;

// init
Font font_init();

void renderText(u32 *Rbuffer, TTF_Font *font, const char *message, int x, int y,
                SDL_Color color);
void renderFloatPair(u32 *Rbuffer, TTF_Font *font, const char *label, double x,
                     double y, int xpos, int ypos, SDL_Color color);
void renderInt(u32 *Rbuffer, TTF_Font *font, const char *label, int value,
               int x, int y, SDL_Color color);
void renderFloat(u32 *Rbuffer, TTF_Font *font, const char *label, double value,
                 int x, int y, SDL_Color color);
void renderProcent(u32 *Rbuffer, TTF_Font *font, int value, int x, int y,
                   SDL_Color color);
#endif
