//#pragma once
//
//#include "common.h"
//
//constexpr int FIXED_SCALE = 128;
//
//inline Vec3 ConvertToFloat(Vec3Fixed v)
//{
//	return { v.x / (float)FIXED_SCALE, v.y / (float)FIXED_SCALE, v.z / (float)FIXED_SCALE };
//}
//
//inline Vec2 ProjectToScreen(Vec3Fixed point, int screenWidth, int screenHeight)
//{
//	return { screenWidth / 2 + point.x, screenHeight / 2 - point.y };
//}
//
//
//
