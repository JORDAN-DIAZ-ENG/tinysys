#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

#include <core.h>

// Screen resolution
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480


// Fixed Point Math
#define FIXED_POINT_SHIFT 12 // Aligns better with 32-bit integer precision (1 << 12 = 4096 scale) // Shift for fixed-point precision (1 << 8 = 256 scale)
#define SCREEN_HALF_WIDTH  int(SCREEN_WIDTH / 2)
#define SCREEN_HALF_HEIGHT  int(SCREEN_HEIGHT / 2)

// Field of View
#define DEFAULT_FOV (500 << FIXED_POINT_SHIFT) // 60-degree FOV in fixed-point

// Video context
extern struct EVideoContext g_videoContext;
extern struct EVideoSwapContext g_swapContext;

struct Mat3
{
	int m[3][3]; // Fixed-Point 3x3 Matrix
};

struct Vec3
{
	int x, y, z;  // 3D Position (Fixed-Point)
};

struct UV
{
	float u, v; // Texture Coordinates 
};

struct EulerRot
{
	int16_t x, y, z;
};

struct Camera
{
	Vec3 position;
	Vec3 forward;
	float fov;
	float nearClip;
	float farClip;
};

// Video Modes
typedef enum
{
	VIDEO_MODE_320x240,
	VIDEO_MODE_640x480
} VideoMode;

// Color Modes
typedef enum
{
	COLOR_MODE_8BIT_INDEXED,
	COLOR_MODE_16BIT_RGB
} ColorMode;

static Camera s_camera = { {0, 0, 1000}, {0, 0, -1}, 1.5f, 1.0f, 11000.0f };

// System Functions
void System_Init(VideoMode mode, ColorMode color);  // System initialization
void System_SwapBuffers();  // Swap framebuffers efficiently
void System_ClearScreen(uint8_t color);  // Fast screen clear
void System_LoadFullPalette(uint16_t *palette);
void System_LoadPartialPalette(uint16_t* palette, int offset, int paletteSize);
void PrecomputeTrigLUT();

#endif // SYSTEM_H
