#include "editor_imgui_backend.h"
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

bool editor_imgui_sdl2_init(SDL_Window *window, void *gl_context)
{
  return ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
}

void editor_imgui_sdl2_shutdown(void)
{
  ImGui_ImplSDL2_Shutdown();
}

void editor_imgui_sdl2_newFrame(void)
{
  ImGui_ImplSDL2_NewFrame();
}

bool editor_imgui_sdl2_processEvent(const SDL_Event *event)
{
  return ImGui_ImplSDL2_ProcessEvent(event);
}

bool editor_imgui_opengl3_init(const char *glsl_version)
{
  return ImGui_ImplOpenGL3_Init(glsl_version);
}

void editor_imgui_opengl3_shutdown(void)
{
  ImGui_ImplOpenGL3_Shutdown();
}

void editor_imgui_opengl3_newFrame(void)
{
  ImGui_ImplOpenGL3_NewFrame();
}

void editor_imgui_opengl3_renderDrawData(ImDrawData *draw_data)
{
  ImGui_ImplOpenGL3_RenderDrawData(draw_data);
}
