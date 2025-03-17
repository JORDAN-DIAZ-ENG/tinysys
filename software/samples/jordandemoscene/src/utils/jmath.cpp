#include "jmath.h"

#include <math.h>

void PrecomputeTrigLUT()
{
	for (int i = 0; i < NUM_ANGLES; i++)
	{
		// Calculate sin and cos and store in LUT
		sinLUT[i] = (int16_t)(sin(i * 3.14f / 180.0) * FIXED_SCALE);
		cosLUT[i] = (int16_t)(cos(i * 3.14f / 180.0) * FIXED_SCALE);
	}
}

// Fast sine function using LUT
int16_t FixedSin(int16_t angle)
{
	// Normalize the angle to be in the range [0, 359]
	angle = angle % 360;
	if (angle < 0) angle += 360;

	return sinLUT[angle];
}

// Fast cosine function using LUT
int16_t FixedCos(int16_t angle)
{
	// Normalize the angle to be in the range [0, 359]
	angle = angle % 360;
	if (angle < 0) angle += 360;

	return cosLUT[angle];
}

//Swap out for rotation matrix later
Vec3 Math_RotateVectorY(Vec3 v, int16_t angle)
{
	int16_t cosA = FixedCos(angle);
	int16_t sinA = FixedSin(angle);

	int16_t x = (v.x * cosA + v.z * sinA) / FIXED_SCALE;
	int16_t z = (-v.x * sinA + v.z * cosA) / FIXED_SCALE;
	return { x, v.y, z };
}


Vec3 Math_RotateVectorX(Vec3 v, int16_t angle)
{
	int16_t cosA = FixedCos(angle);
	int16_t sinA = FixedSin(angle);

	int16_t y = (v.y * cosA - v.z * sinA) / FIXED_SCALE;
	int16_t z = (v.y * sinA + v.z * cosA) / FIXED_SCALE;
	return { v.x, y, z };
}


Vec3 Math_RotateVectorZ(Vec3 v, int16_t angle)
{
	int16_t cosA = FixedCos(angle);
	int16_t sinA = FixedSin(angle);

	int16_t x = (v.x * cosA - v.y * sinA) / FIXED_SCALE;
	int16_t y = (v.x * sinA + v.y * cosA) / FIXED_SCALE;
	return { x, y, v.z };
}


Vec3 Math_ApplyRotation(Vec3 v, EulerRot rot)
{
	// Blender typically applies ZXY or XYZ order, let's match:
	v = Math_RotateVectorZ(v, rot.z);
	v = Math_RotateVectorX(v, rot.x);
	v = Math_RotateVectorY(v, rot.y);
	return v;
}
