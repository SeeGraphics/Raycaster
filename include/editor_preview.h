#ifndef EDITOR_PREVIEW_H
#define EDITOR_PREVIEW_H

#ifdef __cplusplus
extern "C" {
#endif
#include "engine.h"
#ifdef __cplusplus
}
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdbool.h>

typedef struct EditorPreview
{
  Engine engine;
  GLuint sceneTexture;
  int sceneWidth;
  int sceneHeight;
  bool valid;
} EditorPreview;

int editorPreview_init(EditorPreview *preview);
void editorPreview_shutdown(EditorPreview *preview);
void editorPreview_update(EditorPreview *preview, double deltaTime);
void editorPreview_upload(EditorPreview *preview);

#endif
