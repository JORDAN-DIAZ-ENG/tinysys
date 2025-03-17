// draw2d.cpp - Optimized 2D Rendering with VPU Enhancements
#include "draw2d.h"
#include <vpu.h>
#include <core.h>
#include <math.h>

#include "system.h"

// Current drawing buffer
static uint8_t* g_drawBuffer = nullptr;

// Screen dimensions
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

/**
 * @brief Sets the drawing target buffer.
 * @param context Video context
 * @param buffer Framebuffer to draw to
 */
void Render_SetDrawTarget(EVideoContext* context, uint8_t* buffer)
{
    g_drawBuffer = buffer;
    g_videoContext = *context;
    VPUSetWriteAddress(context, (uint32_t)buffer);
}

/**
 * @brief Sets a horizontal blank effect handler.
 * @param handler Function pointer to execute on HBlank
 * @param scanline The scanline to trigger the effect
 */
void Render_SetHBlankEffect(void (*handler)(), int scanline)
{
    VPUSetHBlankHandler((uintptr_t)handler);
    VPUSetHBlankScanline(scanline);
    VPUEnableHBlankInterrupt();
}

/**
 * @brief Sample HBlank effect: Color cycling per scanline
 */
void HBlank_ColorShift()
{
    static uint8_t colorIndex = 0;
    VPUSetPal(colorIndex % 256, (colorIndex & 0xF) << 4, (colorIndex & 0xF0) >> 4, (colorIndex & 0xF00) >> 8);
    colorIndex++;
}

/**
 * @brief Scroll scanout address per scanline for wavy distortion effect
 */
void HBlank_WaveEffect()
{
    static int offset = 0;
    VPUSetScanoutAddress(&g_videoContext, (uint32_t)(g_drawBuffer + (offset % 8)));
    offset++;
}

/**
 * @brief Initializes an example scanline effect.
 */
void Render_InitScanlineEffect()
{
    Render_SetHBlankEffect(HBlank_ColorShift, 100); // Apply color shift at scanline 100
    Render_SetHBlankEffect(HBlank_WaveEffect, 200); // Apply wave distortion at scanline 200
}

/**
 * @brief Generates a plasma effect using sine waves.
 */
void Render_PlasmaEffect()
{
    for (int y = 0; y < SCREEN_HEIGHT; y++)
    {
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            int color = (int)(128 + 127 * sin(x * 0.05) + 127 * sin(y * 0.05));
            g_swapContext.writepage[y * SCREEN_WIDTH + x] = color & 255;
			VPUWaitVSync(); // Wait for vsync and swap buffers
			CFLUSH_D_L1; // Flush cache before swap
			VPUSwapPages(&g_videoContext, &g_swapContext);
        }
    }
}

/**
 * @brief Optimized sprite blitting with transparency handling.
 */
void Render_DrawSprite(int x, int y, const uint8_t* sprite, int width, int height, uint8_t transparentColor, uint8_t paletteOffset = 0)
{
	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < width; col++)
		{
			uint8_t pixel = sprite[row * width + col];
			if (pixel != transparentColor)
			{
				int pos = (y + row) * SCREEN_WIDTH + (x + col);
				g_swapContext.writepage[pos] = pixel + paletteOffset;
			}
		}
	}
}

/**
 * @brief Fast sprite blitting without transparency (memory block copy).
 */
void Render_BlitSprite(int x, int y, const uint8_t* sprite, int width, int height)
{
    uint8_t* dest = &g_swapContext.writepage[y * SCREEN_WIDTH + x];
    for (int row = 0; row < height; row++)
    {
        __builtin_memcpy(dest, &sprite[row * width], width);
        dest += SCREEN_WIDTH;
    }
}


/**
 * @brief Improved fire effect with smoother gradient and decay.
 */
void Render_FireEffect()
{
    for (int x = 0; x < SCREEN_WIDTH; x++)
    {
        g_swapContext.writepage[(SCREEN_HEIGHT - 1) * SCREEN_WIDTH + x] = rand() % 256;
    }
    for (int y = 0; y < SCREEN_HEIGHT - 1; y++)
    {
        for (int x = 1; x < SCREEN_WIDTH - 1; x++)
        {
            int decay = rand() % 3;
            g_swapContext.writepage[y * SCREEN_WIDTH + x] =
                (g_swapContext.writepage[(y + 1) * SCREEN_WIDTH + x] +
                    g_swapContext.writepage[(y + 1) * SCREEN_WIDTH + x - 1] +
                    g_swapContext.writepage[(y + 1) * SCREEN_WIDTH + x + 1] +
                    g_swapContext.writepage[(y + 2) * SCREEN_WIDTH + x]) >> 2;
            g_swapContext.writepage[y * SCREEN_WIDTH + x] = g_swapContext.writepage[y * SCREEN_WIDTH + x] > decay ? g_swapContext.writepage[y * SCREEN_WIDTH + x] - decay : 0;
        }
    }
    for (int i = 0; i < 256; i++)
    {
        VPUSetPal(i, i >> 2, (i >> 1) & 0xF, i & 0xF);

		// Wait for vsync and swap buffers
		VPUWaitVSync();

		// Flush cache before swap
		CFLUSH_D_L1;

		VPUSwapPages(&g_videoContext, &g_swapContext);
    }
}


static uint8_t gradientCache[SCREEN_WIDTH * SCREEN_HEIGHT];
static bool gradientInitialized = false;

/**
 * @brief Simple test effect - Horizontal gradient
 */
void Render_GradientEffect()
{
	if (!gradientInitialized)
	{
		for (int y = 0; y < SCREEN_HEIGHT; y++)
		{
			for (int x = 0; x < SCREEN_WIDTH; x++)
			{
				gradientCache[y * SCREEN_WIDTH + x] = (x * 255) / SCREEN_WIDTH;
			}
		}
		gradientInitialized = true;
	}

	// Copy cached gradient to the framebuffer
	__builtin_memcpy(g_swapContext.writepage, gradientCache, SCREEN_WIDTH * SCREEN_HEIGHT);
	CFLUSH_D_L1; // Ensure cache coherency after drawing
}

/**
 * @brief Draws a line between two points using Bresenham's line algorithm.
 */
void Render_DrawLine(int x0, int y0, int x1, int y1, uint8_t color)
{
	int16_t deltaX = abs(x1 - x0);
	int16_t deltaY = abs(y1 - y0);
	int16_t signX = (x0 < x1) ? 1 : -1;
	int16_t signY = (y0 < y1) ? 1 : -1;

	int16_t error = deltaX - deltaY;
	uint8_t* pixelPtr;

	while (true)
	{
		pixelPtr = &g_swapContext.writepage[y0 * SCREEN_WIDTH + x0];
		*pixelPtr = color;

		if (x0 == x1 && y0 == y1)
			break;

		int16_t error2 = 2 * error;
		if (error2 > -deltaY)
		{
			error -= deltaY;
			x0 += signX;
		}
		if (error2 < deltaX)
		{
			error += deltaX;
			y0 += signY;
		}
	}
}