#include "engine.h"
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

    // Draw the current gun frame
    if (gunAnim.currentAnim && gunAnim.currentAnim[gunAnim.currentFrame]) {
      drawGunTexture(&game, gunAnim.currentAnim[gunAnim.currentFrame], 0.4f,
                     0.6f);
    }

    drawScene(&engine);
    SDL_RenderPresent(engine.game.renderer);
  }

  engine_cleanup(&engine, &soundManager, EXIT_SUCCESS);
  return 0;
}
