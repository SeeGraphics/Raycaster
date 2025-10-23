#include "animation.h"
#include "graphics.h"
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>

SDL_Texture *shotgun_shoot[SHOTGUN_SHOOT_FRAMES] = {NULL};
SDL_Texture *shotgun_reload[SHOTGUN_RELOAD_FRAMES] = {NULL};

/* INIT */
Animation createAnimation() {
  Animation anim = {.currentType = ANIM_SHOTGUN_IDLE,
                    .currentFrame = 0,
                    .frameTime = 0.10,
                    .timeAccumulator = 0.0,
                    .playing = 0,
                    .currentTextures =
                        shotgun_shoot, // Default to shoot idle frame
                    .numFrames = SHOTGUN_SHOOT_FRAMES};
  return anim;
}

/* UPDATE */
void updateAnimation(Animation *anim, double deltaTime) {
  if (!anim->playing) {
    return; // Not playing, nothing to update
  }

  anim->timeAccumulator += deltaTime;

  // Time to advance to next frame?
  if (anim->timeAccumulator >= anim->frameTime) {
    anim->timeAccumulator -= anim->frameTime;
    anim->currentFrame++;

    // Animation finished?
    if (anim->currentFrame >= anim->numFrames) {
      anim->currentFrame = 0;
      anim->playing = 0;

      // Return to idle (shoot frame 0)
      anim->currentType = ANIM_SHOTGUN_IDLE;
      anim->currentTextures = shotgun_shoot;
      anim->numFrames = SHOTGUN_SHOOT_FRAMES;
    }
  }
}

void playAnimation(Animation *anim, AnimationType type) {
  // Dont interrupt current animation
  if (anim->playing) {
    return;
  }

  anim->currentType = type;
  anim->currentFrame = 0;
  anim->timeAccumulator = 0.0;
  anim->playing = 1;

  switch (type) {
  case ANIM_SHOTGUN_SHOOT:
    anim->currentTextures = shotgun_shoot;
    anim->numFrames = SHOTGUN_SHOOT_FRAMES;
    break;

  case ANIM_SHOTGUN_RELOAD:
    anim->currentTextures = shotgun_reload;
    anim->numFrames = SHOTGUN_RELOAD_FRAMES;
    break;

  case ANIM_SHOTGUN_IDLE:
  default:
    anim->currentTextures = shotgun_shoot;
    anim->numFrames = 1; // Just show first frame
    anim->playing = 0;
    break;
  }
}

/* LOAD ANIMATIONS */
SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *filePath) {
  SDL_Surface *surface = IMG_Load(filePath);
  if (!surface) {
    fprintf(stderr, "[ERROR] Failed to load image '%s': %s\n", filePath,
            IMG_GetError());
    return NULL;
  }

  // Set color key for transparency (green #99E550)
  Uint32 green = SDL_MapRGB(surface->format, 0x99, 0xE5, 0x50);
  SDL_SetColorKey(surface, SDL_TRUE, green);

  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  if (!texture) {
    fprintf(stderr, "[ERROR] Failed to create texture for '%s': %s\n", filePath,
            SDL_GetError());
    return NULL;
  }

  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  return texture;
}

void loadAllAnimations(SDL_Renderer *renderer) {
  char filePath[256];

  // Load shoot frames
  for (int i = 0; i < SHOTGUN_SHOOT_FRAMES; i++) {
    snprintf(filePath, sizeof(filePath),
             "assets/textures/weapons/shotgun/shoot/%d.png", i);
    shotgun_shoot[i] = loadTexture(renderer, filePath);
    if (!shotgun_shoot[i]) {
      fprintf(stderr, "[ERROR] Failed to load shotgun shoot frame %d\n", i);
    }
  }

  // Load reload frames
  for (int i = 0; i < SHOTGUN_RELOAD_FRAMES; i++) {
    snprintf(filePath, sizeof(filePath),
             "assets/textures/weapons/shotgun/reload/%d.png", i);
    shotgun_reload[i] = loadTexture(renderer, filePath);
    if (!shotgun_reload[i]) {
      fprintf(stderr, "[ERROR] Failed to load shotgun reload frame %d\n", i);
    }
  }

  printf("[ANIMATION] Loaded shotgun animations\n");
}

/* DRAW ANIMATIONS */
void drawCurrentAnimation(Game *game, Animation *anim, float widthFactor,
                          float heightFactor, float posX, float posY) {

  if (!anim->currentTextures || !anim->currentTextures[anim->currentFrame]) {
    return;
  }

  SDL_Texture *currentTexture = anim->currentTextures[anim->currentFrame];

  int winW = game->window_width;
  int winH = game->window_height;

  // Scale to screen size
  int gunW = (int)(winW * widthFactor);
  int gunH = (int)(winH * heightFactor);

  // Calculate position (centered by default if posX/posY are relative)
  int x = (int)(posX * winW);
  int y = (int)(posY * winH);

  SDL_Rect dstRect = {x, y, gunW, gunH};
  SDL_RenderCopy(game->renderer, currentTexture, NULL, &dstRect);
}

/* CLEANUP */
void cleanupAnimations() {
  for (int i = 0; i < SHOTGUN_SHOOT_FRAMES; i++) {
    if (shotgun_shoot[i]) {
      SDL_DestroyTexture(shotgun_shoot[i]);
      shotgun_shoot[i] = NULL;
    }
  }

  for (int i = 0; i < SHOTGUN_RELOAD_FRAMES; i++) {
    if (shotgun_reload[i]) {
      SDL_DestroyTexture(shotgun_reload[i]);
      shotgun_reload[i] = NULL;
    }
  }
}
