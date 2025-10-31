#include "editor_preview.h"

extern "C" {
#include "animation.h"
#include "entities.h"
#include "map.h"
#include "raycast.h"
#include "sprites.h"
#include "weapons.h"
}
#include <SDL2/SDL_opengl.h>
#include <string.h>

static void editor_engineDefaults(EditorPreview *preview)
{
  memset(&preview->engine, 0, sizeof(Engine));
  preview->engine.mode = GAME;
  preview->engine.game = createGame();
  preview->engine.player = createPlayer();
  preview->engine.textures = createTextures();
  preview->engine.sound = createSound();
}

int editorPreview_init(EditorPreview *preview)
{
  if (!preview)
    return -1;

  memset(preview, 0, sizeof(*preview));
  preview->sceneWidth = RENDER_WIDTH;
  preview->sceneHeight = RENDER_HEIGHT;

  editor_engineDefaults(preview);

  if (buffers_init(&preview->engine.game) != 0)
    return -1;

  if (textures_load(&preview->engine.textures) != 0)
    return -1;

  loadAllAnimations();
  map_loadFromCSV("levels/1/map.csv");
  preview->engine.sprites = entities_createWorldSprites();

  if (!preview->engine.sprites)
    preview->engine.sprites = entities_createWorldSprites();

  glGenTextures(1, &preview->sceneTexture);
  glBindTexture(GL_TEXTURE_2D, preview->sceneTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, preview->sceneWidth,
               preview->sceneHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
  glBindTexture(GL_TEXTURE_2D, 0);

  preview->valid = true;
  return 0;
}

void editorPreview_shutdown(EditorPreview *preview)
{
  if (!preview || !preview->valid)
    return;

  if (preview->sceneTexture)
  {
    glDeleteTextures(1, &preview->sceneTexture);
    preview->sceneTexture = 0;
  }

  freeAllAnimations();

  if (preview->engine.game.Rbuffer)
  {
    free(preview->engine.game.Rbuffer);
    preview->engine.game.Rbuffer = NULL;
  }
  if (preview->engine.game.Zbuffer)
  {
    free(preview->engine.game.Zbuffer);
    preview->engine.game.Zbuffer = NULL;
  }
  if (preview->engine.game.buffer)
  {
    free(preview->engine.game.buffer);
    preview->engine.game.buffer = NULL;
  }

  for (int i = 0; i < NUM_TEXTURES; ++i)
  {
    free(preview->engine.textures.textures[i]);
    preview->engine.textures.textures[i] = NULL;
  }

  preview->valid = false;
}

static void editor_renderScene(EditorPreview *preview)
{
  Engine *engine = &preview->engine;
  clearBuffer(&engine->game);
  perform_floorcasting(engine);
  perform_raycasting(engine);
  perform_spritecasting(engine);
}

void editorPreview_update(EditorPreview *preview, double deltaTime)
{
  if (!preview || !preview->valid)
    return;

  updateAllAnimations(&preview->engine.player, deltaTime);
  editor_renderScene(preview);
}

void editorPreview_upload(EditorPreview *preview)
{
  if (!preview || !preview->valid)
    return;

  glBindTexture(GL_TEXTURE_2D, preview->sceneTexture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, preview->sceneWidth,
                  preview->sceneHeight, GL_BGRA, GL_UNSIGNED_BYTE,
                  preview->engine.game.Rbuffer);
  glBindTexture(GL_TEXTURE_2D, 0);
}
