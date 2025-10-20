#ifndef RAYCAST_H
#define RAYCAST_H

#include "graphics.h"
#include "player.h"
#include "texture.h"

void perform_raycasting(Game *game, TextureManager *tm, Player *player);
void perform_floorcasting(Game *game, TextureManager *tm, Player *player);

#endif
