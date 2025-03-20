// test_scene.cpp - Demo Scene to Test Rendering Effects
#include "system.h"
#include <vpu.h>
#include <core.h>
#include <stdlib.h>
#include <stdio.h>

#include "draw2d.h"
#include "draw3d.h"
#include "mario.inl"
#include "maxwell.inl"
#include "colors.h"
#include "wall.inl"
#include "audio.h"

//#include "explode.inl"
#include "audio.inl"
#include "basesystem.h"
#include "uart.h"
#include "task.h"

void MaxwellMovement(Vec3 &pos, EulerRot &rot, int &scale)
{
	//pos.z++; // causes maxwell to get sucked into a black hole

	////Sin wave up and down movement
	//pos.y += 10 * dir;

	//if (pos.y >= 20)
	//{
	//	dir = -1;
	//}
	//else if (pos.y <= -50)
	//{
	//	dir = 1;
	//}

	////Test_TriangleRender();

	//scale += 10 * scaleDir;

	//if (scale >= 1024)
	//{
	//	scaleDir = -1;
	//}
	//else if (scale <= 512)
	//{
	//	scaleDir = 1;
	//}


	rot.y += 30;
	//maxwellRot.x += 5;

	if (rot.y >= 360)
	{
		rot.y = 0;
	}

	if (rot.x >= 360)
	{
		rot.x = 0;
	}
}


/**
 * @brief Runs the test scene, rendering various effects.
 */
void Test_Scene(void)
{
	System_Init(VIDEO_MODE_640x480, COLOR_MODE_8BIT_INDEXED);

	//System_LoadPartialPalette(s_marioPal, 1, 64);
	//System_LoadPartialPalette(s_mp, 0, 64);
	System_LoadPartialPalette(s_maxwellPalette, 1, 64);
	System_LoadPartialPalette(s_maxwellPalette, 65, 8);
	//System_LoadPartialPalette(s_wallPal, 1, 64);

    int i = 0;
	int xDir = 1;
	int speed = 1;

	//uint8_t paletteOffset = 0;
	int maxwellScale = 1024;
	EulerRot maxwellRot = { -90, 0, -30 };
	Vec3 maxwellPosition = { 0 , -50 , 0  };

	float targetFPS = 1.f;
	uint64_t lastExecutionTime = 0;

    for (;;)
    {

		UpdateDeltaTime();
		uint64_t currentTime = E32ReadTime();
		uint64_t frameTicks = (uint64_t)(ONE_SECOND_IN_TICKS / targetFPS);

		lastExecutionTime = currentTime;

		// Clear screen to black
		System_ClearScreen(150);

		Render_Maxwell(s_maxwellVerts, s_maxwellUVs, s_maxwellNormals, s_maxwellIndecies, 1824, s_maxwellImg, s_maxwellSX, s_maxwellSY, maxwellScale, maxwellRot, 1, maxwellPosition);

		MaxwellMovement(maxwellPosition, maxwellRot, maxwellScale);

		VPUWaitVSync(); // Wait for vsync and swap buffers
		CFLUSH_D_L1; // Flush cache before swap
		VPUSwapPages(&g_videoContext, &g_swapContext);

		if (currentTime - lastExecutionTime >= frameTicks)
		{

			


		}

    }
}