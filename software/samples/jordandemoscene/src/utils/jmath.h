#ifndef JMATH_H
#define JMATH_H

#include "system.h"

// Math
constexpr int FIXED_SCALE = 128;

#define NUM_ANGLES   360  // Number of angles (0° to 359°)

static int16_t sinLUT[NUM_ANGLES];
static int16_t cosLUT[NUM_ANGLES];

void PrecomputeTrigLUT();
int16_t FixedCos(int16_t angle);
int16_t FixedSin(int16_t angle);

Vec3 Math_RotateVectorY(Vec3 v, int16_t angle);
Vec3 Math_RotateVectorX(Vec3 v, int16_t angle);
Vec3 Math_RotateVectorZ(Vec3 v, int16_t angle);
Vec3 Math_ApplyRotation(Vec3 v, EulerRot rot);





#endif // JMATH_H