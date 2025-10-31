#include "engine.h"
#include "graphics.h"
#include "input.h"
#include "enemies.h"
#include "raycast.h"
#include "render.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
  Engine engine;
  if (engine_init(&engine) != 0)
  {
    fprintf(stderr, "\033[31m[ERROR] Engine initialization failed. Exiting.\033[0m\n");
    return EXIT_FAILURE;
  }

  while (true)
  {
    engine_updateTime(&engine);
    handleInput(&engine, engine.deltaTime);

    enemies_update(&engine, engine.deltaTime);
    updateAllAnimations(&engine.player, engine.deltaTime);
    drawScene(&engine);
    SDL_RenderPresent(engine.game.renderer);
  }

  engine_cleanup(&engine, EXIT_SUCCESS);
  return 0;
}
