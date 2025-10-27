#include "engine.h"
#include "raycast.h"
#include "weapons.h"

void drawHud(Engine *engine) {
  /* DEBUG INFO */
  // FPS counter
  renderInt(engine->game.renderer, engine->font.debug, "FPS:", engine->fps, 10,
            10, RGB_Yellow);
  // Coordinates
  renderFloatPair(engine->game.renderer, engine->font.debug,
                  "POS:", engine->player.posX, engine->player.posY, 10, 30,
                  RGB_Yellow);
  // direction
  renderFloatPair(engine->game.renderer, engine->font.debug,
                  "DIR:", engine->player.dirX, engine->player.dirY, 10, 50,
                  RGB_Yellow);
  // pitch
  renderFloat(engine->game.renderer, engine->font.debug,
              "PITCH:", engine->player.pitch, 10, 70, RGB_Yellow);
  // plane
  renderFloatPair(engine->game.renderer, engine->font.debug,
                  "PLANE:", engine->player.planeX, engine->player.planeY, 10,
                  90, RGB_Yellow);

  /* GAME UI */
  // health
  renderProcent(engine->game.renderer, engine->font.ui, engine->player.health,
                engine->game.window_width / 2 - 400,
                engine->game.window_height - 100, RGB_DarkRed);
  // ammo
  if (weaponProperties[engine->player.selectedGun].ammunition != -1) {
    renderInt(engine->game.renderer, engine->font.ui, "",
              weaponProperties[engine->player.selectedGun].ammunition,
              engine->game.window_width - 300, engine->game.window_height - 100,
              RGB_DarkRed);
  }
}

void drawScene(Engine *engine) {

  /* 1. Clear Buffer */
  clearBuffer(&engine->game);

  /* 2. Draw Game */
  perform_floorcasting(engine);
  perform_raycasting(engine);

  if (!engine->game.buffer) {
    fprintf(stderr, "[ERROR] game.buffer is NULL!\n");
  }
  if (!engine->game.Rbuffer) {
    fprintf(stderr, "[ERROR] game.Rbuffer is NULL!\n");
  }
  if (!engine->game.Zbuffer) {
    fprintf(stderr, "[ERROR] game.Zbuffer is NULL!\n");
  }
  perform_spritecasting(engine);
  switch (engine->player.selectedGun) {
  case SHOTGUN:
    blitAnimation(engine->game.Rbuffer, &animations.shotgun_shoot, RENDER_WIDTH,
                  RENDER_HEIGHT, (float)RENDER_WIDTH / 2 - 75,
                  RENDER_HEIGHT - 150, 1.5);
    break;
  case ROCKET:
    blitAnimation(engine->game.Rbuffer, &animations.rocket_shoot, RENDER_WIDTH,
                  RENDER_HEIGHT, (float)RENDER_WIDTH / 2 - 75,
                  RENDER_HEIGHT - 150, 1.5);
    break;
  case PISTOL:
    blitAnimation(engine->game.Rbuffer, &animations.pistol_shoot, RENDER_WIDTH,
                  RENDER_HEIGHT, (float)RENDER_WIDTH / 2 - 75,
                  RENDER_HEIGHT - 150, 1.5);
    break;
  case HANDS:
    blitAnimation(engine->game.Rbuffer, &animations.hands_punsh, RENDER_WIDTH,
                  RENDER_HEIGHT, (float)RENDER_WIDTH / 2 - 150,
                  RENDER_HEIGHT - 150, 1.5);
    break;
  case SINGLE:
    blitAnimation(engine->game.Rbuffer, &animations.single_shoot, RENDER_WIDTH,
                  RENDER_HEIGHT, (float)RENDER_WIDTH / 2 - 75,
                  RENDER_HEIGHT - 150, 1.5);
    break;
  case MINIGUN:
    if (animations.minigun_shoot.playing) {
      blitAnimation(engine->game.Rbuffer, &animations.minigun_shoot,
                    RENDER_WIDTH, RENDER_HEIGHT, (float)RENDER_WIDTH / 2 - 95,
                    RENDER_HEIGHT - 150, 1.5);
    } else {
      blitAnimation(engine->game.Rbuffer, &animations.minigun_idle,
                    RENDER_WIDTH, RENDER_HEIGHT, (float)RENDER_WIDTH / 2 - 95,
                    RENDER_HEIGHT - 150, 1.5);
    }
    break;
  default:
    break;
  }
  drawBuffer(&engine->game);
  drawHud(engine);
}
