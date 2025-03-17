#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdint.h>


#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480     
#define SCREEN_DEPTH 1


#define FIXED_SHIFT 16
#define FIXED_ONE (1 << FIXED_SHIFT)

#define FLOAT_TO_FIXED(x) ((int)((x) * FIXED_ONE))
#define FIXED_TO_INT(x) ((x) >> FIXED_SHIFT)
#define FIXED_MUL(a, b) (((a) * (b)) >> FIXED_SHIFT)
#define FIXED_DIV(a, b) (((a) << FIXED_SHIFT) / (b))

extern struct EVideoContext     g_vx;
extern struct EVideoSwapContext g_sc;

void set320x240(void);
void set640x480(void);
void setPaletteAtIdx(uint8_t idx, uint8_t r, uint8_t g, uint8_t b);
void loadPalette(uint8_t* pal, const int startingIndex, const int paletteSize);
void tickPalette();

//Graphics
struct vec3 { int16_t x, y, z; };
struct vec2 { int16_t x, y; };
struct rotation { int16_t x, y, z; };

struct vec3Textured 
{ 
	int16_t x, y, z;
	float u, v; 
};

struct Vertex2D
{
	int x, y;       // Screen coordinates (integers)
	int u, v;       // Texture coordinates in 16.16 fixed-point
};

struct Tri
{
	vec3 v0, v1, v2;
};

void setPixel(int16_t x, int16_t y, uint32_t color);

void DrawLineFast(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color);

uint32_t GetTextureColor(uint8_t* texture, int textureWidth, int textureHeight, int u, int v);


void DrawTexturedTriangle(Vertex2D v0, Vertex2D v1, Vertex2D v2, uint8_t* texture, int textureWidth, int textureHeight);
void DrawTri(const Tri &triangle, uint32_t color);

void DrawWireframeFast(vec3* vertices, int* indices, int indexCount, uint32_t color, uint16_t scale, rotation& rot);




//static inline void DrawLine(Vec2Fixed p1, Vec2Fixed p2, uint32_t color)
//{
//	int x0 = p1.x, y0 = p1.y;
//	int x1 = p2.x, y1 = p2.y;
//	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
//	int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
//	int err = dx + dy;
//
//
//	while (true)
//	{
//		DrawPixel(buffer, x0, y0, color);
//		if (x0 == x1 && y0 == y1) break;
//		int e2 = 2 * err;
//		if (e2 >= dy) { err += dy; x0 += sx; }
//		if (e2 <= dx) { err += dx; y0 += sy; }
//	}
//}

// Math
//constexpr int FIXED_SCALE_FAST = 128;
//#define NUM_ANGLES   360  // Number of angles (0° to 359°)
//
//// Declare LUTs
//static int16_t sinLUT[NUM_ANGLES];
//static int16_t cosLUT[NUM_ANGLES];
//
//void initTrigLUT();
//int16_t FixedCosFast(int16_t angle);
//int16_t FixedSinFast(int16_t angle);


#endif