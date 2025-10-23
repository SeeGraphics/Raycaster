#ifndef ANIMATION_H
#define ANIMATION_H
#include "graphics.h"
#include <SDL.h>

/* FRAME COUNTS */
#define SHOTGUN_SHOOT_FRAMES 8
#define SHOTGUN_RELOAD_FRAMES 9

/* ANIMATIONS */
extern SDL_Texture *shotgun_shoot[SHOTGUN_SHOOT_FRAMES];
extern SDL_Texture *shotgun_reload[SHOTGUN_RELOAD_FRAMES];

typedef enum {
  ANIM_SHOTGUN_IDLE,
  ANIM_SHOTGUN_SHOOT,
  ANIM_SHOTGUN_RELOAD
} AnimationType;

typedef struct {
  AnimationType currentType;     // Which animation (shoot, reload, idle)
  int currentFrame;              // Which frame we're on (0 to numFrames-1)
  double frameTime;              // Seconds per frame
  double timeAccumulator;        // Time since last frame change
  int playing;                   // 1 = playing, 0 = idle
  SDL_Texture **currentTextures; // Pointer to current animation array
  int numFrames;                 // How many frames in current animation
} Animation;

// Init
Animation createAnimation();

// Update
void updateAnimation(Animation *anim, double deltaTime);
void playAnimation(Animation *anim, AnimationType type);

// Loading
SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *filePath);
void loadAllAnimations(SDL_Renderer *renderer);

// Rendering
void drawCurrentAnimation(Game *game, Animation *anim, float widthFactor,
                          float heightFactor, float posX, float posY);

// Cleanup
void cleanupAnimations();

#endif
