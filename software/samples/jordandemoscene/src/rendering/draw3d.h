#ifndef DRAW3D_H
#define DRAW3D_H

#include <stdint.h>

#include "system.h"

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
