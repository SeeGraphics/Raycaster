#include "engine.h"
#include "raycast.h"

void drawHud(Engine *engine) {
  // if fps doenst work, just do :  int fps = (engine->deltaTime > 0) ?
  // (int)(1.0 / engine->deltaTime) : 0;
  // draw FPS counter
  renderInt(engine->game.renderer, engine->font.debug, "FPS", engine->fps, 10,
            10, RGB_Yellow);
  // draw Coordinates
  renderFloatPair(engine->game.renderer, engine->font.debug, "POS",
                  engine->player.posX, engine->player.posY, 10, 50, RGB_Yellow);
  // draw direction
  renderFloatPair(engine->game.renderer, engine->font.debug, "DIR",
                  engine->player.dirX, engine->player.dirY, 10, 90, RGB_Yellow);
  // draw pitch
  renderFloat(engine->game.renderer, engine->font.debug, "PITCH",
              engine->player.pitch, 10, 170, RGB_Yellow);
  // draw plane
  renderFloatPair(engine->game.renderer, engine->font.debug, "PLANE",
                  engine->player.planeX, engine->player.planeY, 10, 130,
                  RGB_Yellow);
}

void drawScene(Engine *engine) {
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
  drawBuffer(&engine->game);
  drawCurrentAnimation(&engine->game, &engine->animation, 0.4f, 0.6f, 0.3, 0.4);
  drawHud(engine);
}
