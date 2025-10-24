#include "engine.h"
#include "graphics.h"
#include "input.h"
#include "raycast.h"
#include "render.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  Engine engine;
  engine_init(&engine);

  while (true) {
    engine_updateTime(&engine);
    handleInput(&engine, engine.deltaTime);

    updateAnimation(&engine.animation, engine.deltaTime);

    drawScene(&engine);
    SDL_RenderPresent(engine.game.renderer);
  }

  engine_cleanup(&engine, EXIT_SUCCESS);
  return 0;
}
