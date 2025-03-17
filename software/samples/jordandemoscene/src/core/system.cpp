// system.cpp - Optimized System Management with VPU Integration
#include "system.h"
#include "vpu.h"
#include <stdio.h>  // Debugging output

// Double buffering
static uint8_t* g_framebufferA = nullptr;
static uint8_t* g_framebufferB = nullptr;

// Video context
struct EVideoContext g_videoContext;
struct EVideoSwapContext g_swapContext;

/**
 * @brief Initializes the system and sets up video mode using VPU.
 * @param mode Video mode (320x240 or 640x480)
 * @param color Color mode (8-bit indexed or 16-bit RGB)
 */
void System_Init(VideoMode mode, ColorMode color)
{
    // Set color mode
    g_videoContext.m_cmode = (color == COLOR_MODE_8BIT_INDEXED) ? ECM_8bit_Indexed : ECM_16bit_RGB;

    // Set resolution
    g_videoContext.m_vmode = (mode == VIDEO_MODE_320x240) ? EVM_320_Wide : EVM_640_Wide;

    if (!g_framebufferA)
    {
		// Allocate framebuffers using VPU-aligned memory
		g_framebufferA = VPUAllocateBuffer(SCREEN_WIDTH * SCREEN_HEIGHT);
		g_framebufferB = VPUAllocateBuffer(SCREEN_WIDTH * SCREEN_HEIGHT);
    }

    // Assign to swap context
    g_swapContext.framebufferA = g_framebufferA;
    g_swapContext.framebufferB = g_framebufferB;
    g_swapContext.cycle = 0;

    // Apply video settings
    VPUSetVMode(&g_videoContext, EVS_Enable);
    VPUSwapPages(&g_videoContext, &g_swapContext);
    VPUSetDefaultPalette(&g_videoContext); // Ensure default palette is loaded
}

/**
 * @brief Swaps framebuffers for double buffering with VPU sync.
 */
void System_SwapBuffers()
{
    VPUWaitVSync();  // Prevent tearing
    CFLUSH_D_L1; // Discard cache instead of flushing to optimize performance     // Ensure cache coherency
    VPUSwapPages(&g_videoContext, &g_swapContext);
}

/**
 * @brief Clears the active framebuffer with a given color using VPU.
 * @param color The color to fill the screen with.
 */
void System_ClearScreen(uint8_t color)
{
    uint32_t fillColor = (color << 24) | (color << 16) | (color << 8) | color;
    VPUClear(&g_videoContext, fillColor);
    CDISCARD_D_L1; // Discard framebuffer cache to prevent unnecessary writes
}

void System_LoadFullPalette(uint16_t* palette)
{
	for (int i = 0; i < 256; i++)
	{
		uint16_t color = palette[i];  // Read the RGB565 color (16-bit)

		// Extract 12-bit RGB444 from 16-bit RGB565
		uint32_t r = (color >> 8) & 0xF;  // Extract 4-bit Red (Upper 4 bits of 5-bit R)
		uint32_t g = (color >> 4) & 0xF;  // Extract 4-bit Green (Upper 4 bits of 6-bit G)
		uint32_t b = (color >> 0) & 0xF;  // Extract 4-bit Blue (Upper 4 bits of 5-bit B)

		VPUSetPal(i, r, g, b);  // Send converted RGB444 to VPU
	}
}

void System_LoadPartialPalette(uint16_t* palette, int offset, int paletteSize)
{
	for (int i = 0; i < paletteSize; i++)
	{
		uint16_t color = palette[i];  // Read the RGB565 color (16-bit)

		// Extract 12-bit RGB444 from 16-bit RGB565
		uint32_t r = (color >> 8) & 0xF;
		uint32_t g = (color >> 4) & 0xF;
		uint32_t b = (color >> 0) & 0xF;

		VPUSetPal(i + offset, r, g, b);
	}
}

void System_ClearZBuffer()
{
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
	{
		s_zBuffer[i] = 0x7FFF << FIXED_POINT_SHIFT;  // Set to maximum depth value
	}
}