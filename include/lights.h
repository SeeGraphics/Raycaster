#ifndef LIGHTS_H
#define LIGHTS_H

#include "types.h"

void lights_reset(void);
void lights_add(double x, double y, float radius, float intensity,
                float colorR, float colorG, float colorB);
void lights_buildMap(void);
void lights_sampleTile(int tileX, int tileY, float *outBrightness,
                       float *outR, float *outG, float *outB);
void lights_sampleWorld(double x, double y, float *outBrightness,
                        float *outR, float *outG, float *outB);
u32 lights_applyColor(u32 color, float brightness, float tintR, float tintG,
                      float tintB);

#endif
