#ifndef INPUT_H
#define INPUT_H

#include "engine.h"

#define TOGGLE_FULLSCREEN SDL_SCANCODE_7
#define CLOSE_GAME_ESC SDL_SCANCODE_ESCAPE
#define GUN_RELOAD SDL_SCANCODE_R
#define MSB_LEFT SDL_BUTTON_LEFT

int handleInput(Engine *engine, double deltaTime);

#endif
