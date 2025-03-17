#ifndef DRAW3D_H
#define DRAW3D_H

#include <stdint.h>

#include "system.h"

//#include <math.h>

//#include "common.h"
//#include <vector>


//struct Rotation
//{
//	int16_t x, y, z;
//};
//
//struct Vec3Fixed { int16_t x, y, z; };
//struct Vec2Fixed { int16_t x, y; };
//
//
//extern std::vector<Vec2Fixed> modifiedPixels; 
//
//struct Vec3 { float x, y, z; };
//struct Vec2 { float x, y; };
//struct Triangle { Vec3Fixed v0, v1, v2; uint32_t color; };
//
//// Function declarations (no definitions)
//Vec2 ProjectToScreen(Vec3Fixed point, int screenWidth, int screenHeight);
//void DrawLine(uint8_t* buffer, Vec2Fixed p1, Vec2Fixed p2, uint32_t color);
//void DrawWireframeCube(uint8_t* buffer, float size, uint32_t color, float rotationAngle);
//void DrawTriangle(uint8_t* buffer, Triangle t);
//void DrawSolidCube(uint8_t* buffer, float size);
//void DrawWireframe(uint8_t* buffer, Vec3Fixed* vertices, int* indices, int indexCount, uint32_t color, uint16_t scale, Rotation &rot);
//
//void ClearModifiedPixels(uint8_t* bufferA, uint32_t backgroundColor);
//void ClearModifiedPixels(uint8_t* bufferA, uint8_t* bufferB,  uint32_t backgroundColor);

/**
 * @brief Fixed-point multiplication
 */
inline int FixedMul(int a, int b) { return (a * b) >> FIXED_POINT_SHIFT; }

Vec3 ProjectPoint(const Vec3& point, int fov);
Vec3 ProjectVertex(Vec3 point);
void Render_Draw3DLine(const Vec3& p1, const Vec3& p2, uint8_t color);

//3D Model Rendering
void Render_WireFrame(Vec3* vertices, int* indices, int indexCount, uint8_t color, uint16_t scale, EulerRot& rot);
void Render_FilledMesh(Vec3* vertices, int* indices, int indexCount, uint8_t color, uint16_t scale, EulerRot& rot);
void Render_TexturedMesh(Vec3* vertices, UV* uvs, Vec3* normals, int* indices, int indexCount, uint8_t* texture, int texWidth, int texHeight, uint16_t scale, EulerRot& rot, uint8_t paletteOffset);
void Render_Maxwell(Vec3* vertices, UV* uvs, Vec3* normals, int* indices, int indexCount, uint8_t* texture, int texWidth, int texHeight, uint16_t scale, EulerRot& rot, uint8_t paletteOffset, Vec3 position);

//Triangle Filling
void Render_FillTriangle(Vec3 p0, Vec3 p1, Vec3 p2, uint8_t color);
void Render_TextureScanline(int x1, int x2, float u1, float u2, float v1, float v2, int y, uint8_t* texture, int texWidth, int texHeight, uint8_t paletteOffset);
void Render_TextureTriangle(Vec3 p0, Vec3 p1, Vec3 p2, UV uv0, UV uv1, UV uv2, Vec3* normals, uint8_t* texture, int texWidth, int texHeight, uint8_t paletteOffset);
void Render_MaxwellTextureTriangle(Vec3 p0, Vec3 p1, Vec3 p2, UV uv0, UV uv1, UV uv2, Vec3* normals, uint8_t* texture, int texWidth, int texHeight, uint8_t paletteOffset);

#include "draw3d.inl"

#endif // DRAW3D_H
