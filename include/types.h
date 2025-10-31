#ifndef TYPES_H
#define TYPES_H

#include "stdint.h"

typedef uint32_t u32;
typedef int32_t i32;
typedef float f32;
typedef double f64;
typedef struct
{
  i32 x, y;
} v2i;
typedef struct
{
  f32 x, y;
} v2f;

#define LEVEL_COUNT 5

#endif
