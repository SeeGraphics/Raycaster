#ifndef STB_IMAGE_H
#define STB_IMAGE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline unsigned char *stbi_load(const char *filename, int *x, int *y,
                                       int *comp, int req_comp)
{
  (void)req_comp;
  SDL_Surface *surface = IMG_Load(filename);
  if (!surface)
    return NULL;

  SDL_Surface *converted =
      SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(surface);
  if (!converted)
    return NULL;

  *x = converted->w;
  *y = converted->h;
  *comp = 4;

  size_t size = (size_t)(converted->pitch * converted->h);
  unsigned char *pixels = (unsigned char *)malloc(size);
  if (!pixels)
  {
    SDL_FreeSurface(converted);
    return NULL;
  }

  memcpy(pixels, converted->pixels, size);
  SDL_FreeSurface(converted);
  return pixels;
}

static inline void stbi_image_free(void *retval_from_stbi_load)
{
  free(retval_from_stbi_load);
}

#ifdef __cplusplus
}
#endif

#endif /* STB_IMAGE_H */
