#include "engine.h"
#include "input.h"
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

  bool running = true;
  while (running)
  {
    engine_updateTime(&engine);
    if (handleInput(&engine, engine.deltaTime))
    {
      running = false;
      break;
    }
    engine_frame(&engine);
  }

  engine_cleanup(&engine, EXIT_SUCCESS);
  return 0;
}
