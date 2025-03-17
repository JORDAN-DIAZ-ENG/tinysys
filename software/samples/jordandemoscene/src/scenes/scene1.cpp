#include "common.h"
#include "config.h"
#include "colors.h"
#include "draw2d.h"
#include "draw3d.h"

#include "mario.inl"

#include "core.h"
#include "vpu.h"




void scene1( void )
{
	//set640x480();


	//float rotationAngle = 0.0f;
	//float rotationAngle2 = 0.0f;

	////Define Palette
	//loadPalette(s_marioPal, 64, 64);
	//int marioX = 0;
	//int marioY = 0;
	//int marioAccelerationX = 20;
	//int marioAccelerationY = 20;

	//// main loop
	//for (;;)
	//{
	//	VPUClear(&g_vx, SOLIDBLACK);

	//	rotationAngle += 0.03f;
	//	if (rotationAngle >= 2 * 3.14f) rotationAngle = 0;

	//	rotationAngle2 -= 0.08f;
	//	if (rotationAngle2 >= 2 * 3.14f) rotationAngle2 = 0;

	//	uint8_t* currentBuffer = ((g_sc.cycle) % 2) ? g_sc.framebufferA : g_sc.framebufferB;

	//	if (marioX >= 640 - s_marioSX || marioX < 0)
	//	{
	//		marioX = (marioX >= 640 - s_marioSX) ? 640 - s_marioSX : 0;
	//		marioAccelerationX = -marioAccelerationX;
	//	}

	//	if (marioY >= 480 - s_marioSY || marioY < 0)
	//	{
	//		marioY = (marioY >= 480 - s_marioSY) ? 480 - s_marioSY : 0;
	//		marioAccelerationY = -marioAccelerationY;
	//	}

	//	// Draw Mario
	//	Draw2D8bitIndexedSquare(currentBuffer, s_marioImg, marioX, marioY, s_marioSX, s_marioSY, 64);

	//	//Draw2D8bitIndexedSquare(currentBuffer, s_marioImg, 0, marioY, s_marioSX, s_marioSY, 0); // 2 marios is slower than 1 mario
	//	marioX += marioAccelerationX;
	//	marioY += marioAccelerationY;


	//	//VPUSetDefaultPalette(&g_vx);
	//	DrawWireframeCube(currentBuffer, 50, SOLIDBLUE, rotationAngle);
	//	DrawWireframeCube(currentBuffer, 100, SOLIDWHITE, rotationAngle2);
	//	//Draw2DSquareToCurrentBuffer(&g_sc, SOLIDRED, 100, 100, 50);

	//	VPUWaitVSync();
	//	CFLUSH_D_L1;
	//	VPUSwapPages(&g_vx, &g_sc);
	//	//E32Sleep(0.5f * ONE_SECOND_IN_TICKS);

	//}
}