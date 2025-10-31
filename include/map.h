#ifndef MAP_H
#define MAP_H

#define MAP_WIDTH 24
#define MAP_HEIGHT 24

#ifdef __cplusplus
extern "C" {
#endif

extern int worldMap[MAP_WIDTH][MAP_HEIGHT];
int map_loadFromCSV(const char *filepath);
void map_resetToDefault(void);

#ifdef __cplusplus
}
#endif

#endif
