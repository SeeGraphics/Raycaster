#ifndef RAYCAST_H
#define RAYCAST_H

typedef struct Engine Engine;

void perform_raycasting(Engine *engine);
void perform_floorcasting(Engine *engine);

extern int g_floorTextureId;
extern int g_ceilingTextureId;

#endif
