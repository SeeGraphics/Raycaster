#include "map.h"
#include "types.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int defaultMap[MAP_WIDTH][MAP_HEIGHT] = {
    {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8},
    {8, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
    {8, 0, 6, 6, 6, 0, 0, 0, 0, 3, 0, 3, 0, 0, 4, 4, 4, 0, 0, 0, 5, 5, 0, 8},
    {8, 0, 6, 0, 6, 0, 0, 0, 0, 3, 0, 3, 0, 0, 4, 0, 4, 0, 0, 0, 5, 0, 0, 8},
    {8, 0, 6, 0, 6, 0, 0, 7, 7, 7, 0, 3, 0, 0, 4, 0, 4, 0, 0, 0, 5, 0, 0, 8},
    {8, 0, 6, 0, 6, 0, 0, 7, 0, 7, 0, 3, 0, 0, 4, 0, 4, 0, 0, 0, 5, 0, 0, 8},
    {8, 0, 6, 0, 6, 0, 0, 7, 7, 7, 0, 3, 0, 0, 4, 4, 4, 0, 0, 0, 5, 5, 0, 8},
    {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
    {8, 0, 0, 0, 0, 9, 9, 9, 0, 0, 0, 0, 0, 10, 10, 10, 0, 0, 0, 11, 11, 11, 0, 8},
    {8, 0, 0, 0, 0, 9, 0, 9, 0, 0, 0, 0, 0, 10, 0, 10, 0, 0, 0, 11, 0, 11, 0, 8},
    {8, 0, 0, 0, 0, 9, 9, 9, 0, 0, 0, 0, 0, 10, 10, 10, 0, 0, 0, 11, 11, 11, 0, 8},
    {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
    {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 12, 12, 0, 0, 0, 0, 13, 13, 13, 0, 0, 0, 8},
    {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 12, 0, 0, 0, 0, 13, 0, 13, 0, 0, 0, 8},
    {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 12, 12, 0, 0, 0, 0, 13, 13, 13, 0, 0, 0, 8},
    {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
    {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
    {8, 0, 2, 2, 2, 0, 0, 0, 0, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 8},
    {8, 0, 2, 0, 2, 0, 0, 0, 0, 6, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 3, 8},
    {8, 0, 2, 0, 2, 0, 0, 0, 0, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 8},
    {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
    {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
    {8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
    {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8}};

static int mapInitialized = 0;
int worldMap[MAP_WIDTH][MAP_HEIGHT];

static void map_applyDefault(void)
{
  memcpy(worldMap, defaultMap, sizeof(worldMap));
  mapInitialized = 1;
}

void map_resetToDefault(void)
{
  map_applyDefault();
}

static void map_ensureInitialized(void)
{
  if (!mapInitialized)
    map_applyDefault();
}

int map_loadFromCSV(const char *filepath)
{
  map_ensureInitialized();

  FILE *file = fopen(filepath, "r");
  if (!file)
  {
    fprintf(stderr, "\033[31m[ERROR] Failed to open map file '%s': %s\033[0m\n",
            filepath, strerror(errno));
    map_applyDefault();
    return -1;
  }

  char buffer[512];
  int row = 0;

  while (row < MAP_HEIGHT && fgets(buffer, sizeof(buffer), file))
  {
    char *line = buffer;
    while (*line && isspace((unsigned char)*line))
      line++;
    if (*line == '\0')
      continue;

    int col = 0;
    char *token = strtok(line, ",");
    while (token)
    {
      char *endPtr = NULL;
      long value = strtol(token, &endPtr, 10);
      if (token == endPtr)
      {
        fprintf(stderr,
                "\033[31m[ERROR] Invalid map entry '%s' at row %d\033[0m\n",
                token, row);
        fclose(file);
        map_applyDefault();
        return -1;
      }

      if (col >= MAP_WIDTH)
      {
        fprintf(stderr,
                "\033[31m[ERROR] Too many columns in map row %d (max %d)\033[0m\n",
                row, MAP_WIDTH);
        fclose(file);
        map_applyDefault();
        return -1;
      }

      worldMap[col][row] = (int)value;
      col++;
      token = strtok(NULL, ",");
    }

    if (col != MAP_WIDTH)
    {
      fprintf(stderr,
              "\033[31m[ERROR] Map row %d has %d columns, expected %d\033[0m\n",
              row, col, MAP_WIDTH);
      fclose(file);
      map_applyDefault();
      return -1;
    }

    row++;
  }

  if (row != MAP_HEIGHT)
  {
    fprintf(stderr,
            "\033[31m[ERROR] Map file '%s' contains %d rows, expected %d\033[0m\n",
            filepath, row, MAP_HEIGHT);
    fclose(file);
    map_applyDefault();
    return -1;
  }

  fclose(file);
  return 0;
}
