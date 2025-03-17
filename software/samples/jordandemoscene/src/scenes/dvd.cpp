#include "common.h"
#include "config.h"
#include "colors.h"
#include "draw2d.h"
#include "draw3d.h"

#include "dvd_logo.inl"

#include "core.h"
#include "vpu.h"

//Palette index for the background
constexpr uint32_t PALETTE_COLOR_ONE = ((0 << 24) | (0 << 16) | (0 << 8) | 0); // First color in the palette, typically black if left untouched

void dvdScene( void )
{
	// Start
	set640x480();

	// Init Palette
	loadPalette(s_dvdPal, 1, 2);

	// Wait 5 Seconds

	// Draw the Logo in the center of the screen

	int dvdX = 0;
	int dvdY = 0;
	int dvdAccelerationX = 10;
	int dvdAccelerationY = 5;

	uint8_t backgroundOffset = 0;

	for (;;)
	{
		//Clear
		VPUClear(&g_vx, PALETTE_COLOR_ONE);


		//Update

		if (dvdX >= 640 - s_dvdSX || dvdX < 0)
		{
			dvdX = (dvdX >= 640 - s_dvdSX) ? 640 - s_dvdSX : 0;
			dvdAccelerationX = -dvdAccelerationX;
			s_dvdPal[1]++;
			backgroundOffset++;
			setPaletteAtIdx(0, (PALETTE_COLOR_ONE >> 24) + backgroundOffset, (PALETTE_COLOR_ONE >> 16) + backgroundOffset, (PALETTE_COLOR_ONE >> 8) + backgroundOffset);
			loadPalette(s_dvdPal, 1, 2);
		}

		if (dvdY >= 480 - s_dvdSY || dvdY < 0)
		{
			dvdY = (dvdY >= 480 - s_dvdSY) ? 480 - s_dvdSY : 0;
			dvdAccelerationY = -dvdAccelerationY;
			s_dvdPal[1]++;
			backgroundOffset++;
			setPaletteAtIdx(0, (PALETTE_COLOR_ONE >> 24) + backgroundOffset, (PALETTE_COLOR_ONE >> 16) + backgroundOffset, (PALETTE_COLOR_ONE >> 8) + backgroundOffset);
			loadPalette(s_dvdPal, 1, 2);
		}

		

		//Draw
		Draw2D8bitIndexedSquare(g_sc.writepage, s_dvdImg, dvdX, dvdY, s_dvdSX, s_dvdSY, 1);

		//Post Update
		dvdX += dvdAccelerationX;
		dvdY += dvdAccelerationY;

		VPUWaitVSync();
		CFLUSH_D_L1;
		VPUSwapPages(&g_vx, &g_sc);
	}



	// wait 5 seconds

	// have the logo bounce around the screen

}