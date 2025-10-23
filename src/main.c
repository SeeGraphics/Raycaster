#include "engine.h"
#include "font.h"
#include "graphics.h"
#include "gun.h"
#include "input.h"
#include "raycast.h"
#include "sprites.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  Engine engine;
  engine_init(&engine);

  while (true) {

    engine_updateTime(&engine);
    handleInput(&engine, engine.deltaTime);

    // TODO: GUNANIMATION REWRITE HELPER FUNCTION updateAnimations() & Call in
    // handleInput() update animation
    if (gunAnim.playing) {
      gunAnim.timeAccumulator += deltaTime;
      if (gunAnim.timeAccumulator >= gunAnim.frameTime) {
        gunAnim.timeAccumulator -= gunAnim.frameTime;
        gunAnim.currentFrame++;

        // Stop animation when finished
        if (gunAnim.currentFrame >= gunAnim.maxFrames) {
          gunAnim.currentFrame = 0;
          gunAnim.playing = 0;

          gunAnim.currentAnim = gunTextures;
          gunAnim.maxFrames = SHOTGUN_SHOOT_FRAMES;
        }
      }
    }

    // TODO: Put in render.c and drawScene();
    // Clear buffer
    for (int y = 0; y < game.window_height; y++) {
      for (int x = 0; x < game.window_width; x++) {
        game.buffer[y * game.window_width + x] = 0;
      }
    }

    // draw game
    perform_floorcasting(&game, &textureManager, &player);
    perform_raycasting(&game, &textureManager, &player);
    perform_spritecasting(&game, sprite, &textureManager, &player);
    drawBuffer(&game);

    // Draw the current gun frame
    if (gunAnim.currentAnim && gunAnim.currentAnim[gunAnim.currentFrame]) {
      drawGunTexture(&game, gunAnim.currentAnim[gunAnim.currentFrame], 0.4f,
                     0.6f);
    }

    // draw FPS counter
    renderInt(game.renderer, font.debug, "FPS", fps, 10, 10, RGB_Yellow);
    // draw Coordinates
    renderFloatPair(game.renderer, font.debug, "POS", player.posX, player.posY,
                    10, 50, RGB_Yellow);
    // draw direction
    renderFloatPair(game.renderer, font.debug, "DIR", player.dirX, player.dirY,
                    10, 90, RGB_Yellow);
    // draw pitch
    renderFloat(game.renderer, font.debug, "PITCH", player.pitch, 10, 170,
                RGB_Yellow);
    // draw plane
    renderFloatPair(game.renderer, font.debug, "PLANE", player.planeX,
                    player.planeY, 10, 130, RGB_Yellow);

    SDL_RenderPresent(game.renderer);
  }

  engine_cleanup(&engine, &soundManager, EXIT_SUCCESS);
  return 0;
}
