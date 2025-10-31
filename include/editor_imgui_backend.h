#ifndef EDITOR_IMGUI_BACKEND_H
#define EDITOR_IMGUI_BACKEND_H

#include <stdbool.h>
#include <SDL2/SDL.h>

struct ImDrawData;

bool editor_imgui_sdl2_init(SDL_Window *window, void *gl_context);
void editor_imgui_sdl2_shutdown(void);
void editor_imgui_sdl2_newFrame(void);
bool editor_imgui_sdl2_processEvent(const SDL_Event *event);

bool editor_imgui_opengl3_init(const char *glsl_version);
void editor_imgui_opengl3_shutdown(void);
void editor_imgui_opengl3_newFrame(void);
void editor_imgui_opengl3_renderDrawData(struct ImDrawData *draw_data);

#endif
