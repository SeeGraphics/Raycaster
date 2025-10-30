#include "engine.h"
#include "raycast.h"
#include "weapons.h"

void drawDebugHUD(Engine *engine) {
  // FPS counter
  renderInt(engine->game.Rbuffer, engine->font.debug, "FPS:", engine->fps, 10,
            0, RGB_Yellow);
  // Coordinates
  renderf32Pair(engine->game.Rbuffer, engine->font.debug,
                  "POS:", engine->player.posX, engine->player.posY, 10, 15,
                  RGB_Yellow);
  // direction
  renderf32Pair(engine->game.Rbuffer, engine->font.debug,
                  "DIR:", engine->player.dirX, engine->player.dirY, 10, 30,
                  RGB_Yellow);
  // pitch
  renderf32(engine->game.Rbuffer, engine->font.debug,
              "PITCH:", engine->player.pitch, 10, 45, RGB_Yellow);
  // plane
  renderf32Pair(engine->game.Rbuffer, engine->font.debug,
                  "PLANE:", engine->player.planeX, engine->player.planeY, 10,
                  60, RGB_Yellow);
}

void drawGameHUD(Engine *engine) {
  // health
  renderProcent(engine->game.Rbuffer, engine->font.ui, engine->player.health,
                RENDER_WIDTH / 2 - 250, RENDER_HEIGHT - 40, RGB_DarkRed);
  // ammo
  if (weaponProperties[engine->player.selectedGun].ammunition != -1) {
    renderInt(engine->game.Rbuffer, engine->font.ui, "",
              weaponProperties[engine->player.selectedGun].ammunition,
              RENDER_WIDTH / 2 + 200, RENDER_HEIGHT - 40, RGB_DarkRed);
  }
}

void drawDebug(Engine *engine) {
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
                  RENDER_HEIGHT, (f32)RENDER_WIDTH / 2 - 75,
                  RENDER_HEIGHT - 150, 1.5);
    break;
  case ROCKET:
    blitAnimation(engine->game.Rbuffer, &animations.rocket_shoot, RENDER_WIDTH,
                  RENDER_HEIGHT, (f32)RENDER_WIDTH / 2 - 75,
                  RENDER_HEIGHT - 150, 1.5);
    break;
  case PISTOL:
    blitAnimation(engine->game.Rbuffer, &animations.pistol_shoot, RENDER_WIDTH,
                  RENDER_HEIGHT, (f32)RENDER_WIDTH / 2 - 75,
                  RENDER_HEIGHT - 150, 1.5);
    break;
  /* case HANDS: */
  /*   blitAnimation(engine->game.Rbuffer, &animations.hands_punsh,
   * RENDER_WIDTH, */
  /*                 RENDER_HEIGHT, (f32)RENDER_WIDTH / 2 - 150, */
  /*                 RENDER_HEIGHT - 150, 1.5); */
  /*   break; */
  case SINGLE:
    blitAnimation(engine->game.Rbuffer, &animations.single_shoot, RENDER_WIDTH,
                  RENDER_HEIGHT, (f32)RENDER_WIDTH / 2 - 75,
                  RENDER_HEIGHT - 150, 1.5);
    break;
  case MINIGUN:
    if (animations.minigun_shoot.playing) {
      blitAnimation(engine->game.Rbuffer, &animations.minigun_shoot,
                    RENDER_WIDTH, RENDER_HEIGHT, (f32)RENDER_WIDTH / 2 - 95,
                    RENDER_HEIGHT - 150, 1.5);
    } else {
      blitAnimation(engine->game.Rbuffer, &animations.minigun_idle,
                    RENDER_WIDTH, RENDER_HEIGHT, (f32)RENDER_WIDTH / 2 - 95,
                    RENDER_HEIGHT - 150, 1.5);
    }
    break;
  default:
    break;
  }
  drawDebugHUD(engine);
  drawGameHUD(engine);
  drawBuffer(&engine->game);
}

void drawGame(Engine *engine) {

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
                  RENDER_HEIGHT, (f32)RENDER_WIDTH / 2 - 75,
                  RENDER_HEIGHT - 150, 1.5);
    break;
  case ROCKET:
    blitAnimation(engine->game.Rbuffer, &animations.rocket_shoot, RENDER_WIDTH,
                  RENDER_HEIGHT, (f32)RENDER_WIDTH / 2 - 75,
                  RENDER_HEIGHT - 150, 1.5);
    break;
  case PISTOL:
    blitAnimation(engine->game.Rbuffer, &animations.pistol_shoot, RENDER_WIDTH,
                  RENDER_HEIGHT, (f32)RENDER_WIDTH / 2 - 75,
                  RENDER_HEIGHT - 150, 1.5);
    break;
  /* case HANDS: */
  /*   blitAnimation(engine->game.Rbuffer, &animations.hands_punsh,
   * RENDER_WIDTH, */
  /*                 RENDER_HEIGHT, (f32)RENDER_WIDTH / 2 - 150, */
  /*                 RENDER_HEIGHT - 150, 1.5); */
  /*   break; */
  case SINGLE:
    blitAnimation(engine->game.Rbuffer, &animations.single_shoot, RENDER_WIDTH,
                  RENDER_HEIGHT, (f32)RENDER_WIDTH / 2 - 75,
                  RENDER_HEIGHT - 150, 1.5);
    break;
  case MINIGUN:
    if (animations.minigun_shoot.playing) {
      blitAnimation(engine->game.Rbuffer, &animations.minigun_shoot,
                    RENDER_WIDTH, RENDER_HEIGHT, (f32)RENDER_WIDTH / 2 - 95,
                    RENDER_HEIGHT - 150, 1.5);
    } else {
      blitAnimation(engine->game.Rbuffer, &animations.minigun_idle,
                    RENDER_WIDTH, RENDER_HEIGHT, (f32)RENDER_WIDTH / 2 - 95,
                    RENDER_HEIGHT - 150, 1.5);
    }
    break;
  default:
    break;
  }
  drawGameHUD(engine);
  drawBuffer(&engine->game);
}

void drawScene(Engine *engine) {
  switch (engine->mode) {
  case GAME:
    drawGame(engine);
    break;
  case DEBUG:
    drawDebug(engine);
    break;
  }
}
