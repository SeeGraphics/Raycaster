#include "render.h"

void drawScene(Engine *engine) {
  /* 1. Clear Buffer */
  for (int y = 0; y < engine->game.window_height; y++) {
    for (int x = 0; x < engine->game.window_width; x++) {
      engine->game.buffer[y * engine->game.window_width + x] = 0;
    }
  }

  /* 2. Draw Game */
  perform_floorcasting(&engine->game);
  perform_raycasting(&engine->game);
  perform_spritecasting(&engine->game);
  drawBuffer(&engine->game);
}
