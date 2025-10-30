#include "animation.h"
#include "SDL_surface.h"
#include "player.h"
#include "types.h"
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>

AnimationRegistry animations;

Frame loadFrame(const char *path) {

  SDL_Surface *surface = IMG_Load(path);
  if (!surface)
    fprintf(stderr, "\033[31m[ERROR] Failed to create Surface:  %s\033[0m\n",
            SDL_GetError());

  u32 green = SDL_MapRGB(surface->format, 0x99, 0xE5, 0x50);
  SDL_SetColorKey(surface, SDL_TRUE, green);

  SDL_Surface *converted =
      SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
  if (!converted)
    fprintf(stderr, "\033[31m[ERROR] Failed to create Surface:  %s\033[0m\n",
            SDL_GetError());

  int width = converted->w;
  int height = converted->h;

  u32 *pixels = malloc(width * height * sizeof(u32));
  if (!pixels)
    fprintf(stderr, "\033[31m[ERROR] Failed to allocate buffer:  %s\033[0m\n",
            SDL_GetError());
  u32 *image = (u32 *)converted->pixels;
  if (!image)
    fprintf(stderr, "\033[31m[ERROR] Failed to allocate buffer:  %s\033[0m\n",
            SDL_GetError());

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int image_index = y * (converted->pitch / 4) + x;
      int dstindex = y * width + x;

      u32 color = image[image_index];
      pixels[dstindex] = color;
    }
  }

  SDL_FreeSurface(surface);
  SDL_FreeSurface(converted);

  Frame frame = {pixels, width, height};
  return frame;
}

Animation loadAnimation(const char *folder, int frameCount, double frameTime,
                        int playing, int looping) {
  Animation animation;
  animation.playing = playing;
  animation.timeAccumulator = 0;
  animation.frameCount = frameCount;
  animation.frameTime = frameTime;
  animation.currentFrame = 0;
  animation.looping = looping;
  animation.frames = malloc(frameCount * sizeof(Frame));
  if (!animation.frames)
    fprintf(stderr, "\033[31m[ERROR] Failed to allocate buffer: %s\033[0m\n",
            SDL_GetError());

  for (int i = 0; i < frameCount; i++) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%d.png", folder, i);
    animation.frames[i] = loadFrame(path);
  }

  return animation;
}

void loadAllAnimations() { // 0 playing, 0 looping
  animations.shotgun_shoot =
      loadAnimation("assets/textures/weapons/shotgun/shoot",
                    FRAMES_SHOTGUN_SHOOT, FRAMETIME_SHOTGUN, 0, 0);
  animations.rocket_shoot =
      loadAnimation("assets/textures/weapons/rocket/shoot", FRAMES_ROCKET_SHOOT,
                    FRAMETIME_ROCKET, 0, 0);
  animations.pistol_shoot =
      loadAnimation("assets/textures/weapons/pistol/shoot", FRAMES_PISTOL_SHOOT,
                    FRAMETIME_PISTOL, 0, 0);
  animations.hands_punsh =
      loadAnimation("assets/textures/weapons/hands/punsh", FRAMES_HANDS_PUNSH,
                    FRAMETIME_HANDS, 0, 0);
  animations.single_shoot =
      loadAnimation("assets/textures/weapons/single_shotgun/shoot",
                    FRAMES_SINGLE_SHOOT, FRAMETIME_SINGLE_SHOOT, 0, 0);
  animations.minigun_shoot =
      loadAnimation("assets/textures/weapons/minigun/shoot",
                    FRAMES_MINIGUN_SHOOT, FRAMETIME_MINIGUN_SHOOT, 0, 1);
  animations.minigun_idle =
      loadAnimation("assets/textures/weapons/minigun/idle", FRAMES_MINIGUN_IDLE,
                    FRAMETIME_MINIGUN_IDLE, 0, 0);
  animations.demon_walk =
      loadAnimation("assets/textures/entities/demon/walk", FRAMES_DEMON_WALK,
                    FRAMETIME_DEMON_WALK, 1, 1);
}

void updateAnimation(Animation *animation, Player *player, double deltaTime) {
  if (!animation)
    return;

  if (!animation->playing) {
    if (player)
      player->shooting = 0;
    return;
  }

  if (player)
    player->shooting = 1;

  animation->timeAccumulator += deltaTime;
  if (animation->timeAccumulator < animation->frameTime)
    return;

  animation->timeAccumulator -= animation->frameTime;
  animation->currentFrame++;

  if (animation->currentFrame < animation->frameCount)
    return;

  if (player)
    player->shooting = 0;

  if (animation->looping) {
    animation->currentFrame = 0;
    return;
  }

  animation->currentFrame = 0;
  animation->playing = 0;
}

void updateAllAnimations(Player *player, double deltaTime) {
  updateAnimation(&animations.shotgun_shoot, player, deltaTime);
  updateAnimation(&animations.rocket_shoot, player, deltaTime);
  updateAnimation(&animations.pistol_shoot, player, deltaTime);
  updateAnimation(&animations.hands_punsh, player, deltaTime);
  updateAnimation(&animations.single_shoot, player, deltaTime);
  updateAnimation(&animations.minigun_shoot, player, deltaTime);
  updateAnimation(&animations.minigun_idle, player, deltaTime);
  updateAnimation(&animations.demon_walk, NULL, deltaTime);
}

void blitFrame(u32 *buffer, Frame *frame, f32 width, f32 height, f32 x,
               f32 y, f32 scale) {

  int scaled_height = (int)frame->height * scale;
  int scaled_width = (int)frame->width * scale;
  for (int dsty = 0; dsty < scaled_height; dsty++) {
    for (int dstx = 0; dstx < scaled_width; dstx++) {
      int imgy = (int)dsty / scale;
      int imgx = (int)dstx / scale;

      int screenX = dstx + x;
      int screenY = dsty + y;

      if (screenX < 0 || screenX >= width || screenY < 0 || screenY >= height) {
        continue;
      }

      int image_index = imgy * frame->width + imgx;
      u32 color = frame->pixels[image_index];

      if ((color & 0xFF000000) == 0) {
        continue;
      }

      int dstIndex = screenY * width + screenX;
      buffer[dstIndex] = color;
    }
  }
}

void blitAnimation(u32 *buffer, Animation *animation, f32 width, f32 height,
                   f32 x, f32 y, f32 scale) {
  Frame *currentFrame = &animation->frames[animation->currentFrame];

  blitFrame(buffer, currentFrame, width, height, x, y, scale);
}

void freeFrame(Frame *frame) {
  if (frame->pixels) {
    free(frame->pixels);
    frame->pixels = NULL;
  }
}

void freeAnimation(Animation *animation) {
  if (!animation || !animation->frames)
    return;

  //  free frames
  for (int i = 0; i < animation->frameCount; i++) {
    if (animation->frames[i].pixels) {
      free(animation->frames[i].pixels);
      animation->frames[i].pixels = NULL;
    }
  }

  // Free the arrays
  free(animation->frames);
  animation->frames = NULL;
}

void freeAllAnimations() {
  freeAnimation(&animations.rocket_shoot);
  freeAnimation(&animations.shotgun_shoot);
  freeAnimation(&animations.pistol_shoot);
  freeAnimation(&animations.single_shoot);
  freeAnimation(&animations.hands_punsh);
  freeAnimation(&animations.minigun_shoot);
  freeAnimation(&animations.minigun_idle);
  freeAnimation(&animations.demon_walk);
}
