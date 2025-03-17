//#include "common.h"
//#include "config.h"
//#include "colors.h"
//#include "draw2d.h"
//#include "draw3d.h"
//
//#include "maxwell.inl"
//
//#include "core.h"
//#include "vpu.h"
//
//#include "mario.inl"
//
//#define TEXTURE_WIDTH 256
//#define TEXTURE_HEIGHT 256
//
//Vec3Fixed vertices[] = {
//	{ -500, -500, -500 },  // Vertex 0
//	{ 500, -500, -500 },   // Vertex 1
//	{ 0, 500, -500 },      // Vertex 2
//	{ -500, 500, -500 },   // Vertex 3 (new vertex for second triangle)
//	{ 500, 500, -500 },    // Vertex 4 (new vertex for second triangle)
//};
//
//// Indices for a single triangle
//static int indices[] = {
//	0, 1, 2,
//	2, 1, 4// Triangle (vertex 0, 1, 2)
//};
//
//void scene2( void )
//{
//	set640x480();
//
//
//	int indexCount = sizeof(indices) / sizeof(indices[0]);
//
//	int scale = 16;
//
//	Rotation rot = { 0, 0, 0 };
//	rotation rotFast = { 0, 0, 0 };
//
//	uint8_t s_texture[TEXTURE_WIDTH * TEXTURE_HEIGHT * 3]; // RGB texture
//
//	uint32_t greenColor = 0x00FF00; // Green color, in 32-bit format
//	for (int i = 0; i < TEXTURE_WIDTH * TEXTURE_HEIGHT; ++i)
//	{
//		s_texture[i] = greenColor; // Set every pixel to green
//	}
//
//	const int textureWidth = 8;
//	const int textureHeight = 8;
//
//	uint8_t blueTexture[textureWidth * textureHeight] = {
//	1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1
//	};
//
//	Tri exampleTriangle = {
//	{ 100, 100, 10 },  // Vertex 0 (v0) - Position: (100, 100, 0)
//	{ 200, 100, 0 },  // Vertex 1 (v1) - Position: (200, 100, 0)
//	{ 150, 200, 0 }   // Vertex 2 (v2) - Position: (150, 200, 0)
//	};
//
//	// main loop
//	for (;;)
//	{
//		VPUClear(&g_vx, SOLIDBLACK);
//		
//		uint8_t* currentBuffer = ((g_sc.cycle) % 2) ? g_sc.framebufferA : g_sc.framebufferB;
//
//		ClearModifiedPixels(currentBuffer, SOLIDBLACK);
//		
//		//DrawWireframeFast(s_maxwellVerts, s_maxwellIndecies, 1824, SOLIDWHITE, scale, rotFast);
//
//		Vertex2D v0 = { 50, 50, FLOAT_TO_FIXED(0.0f), FLOAT_TO_FIXED(1.0f) };   // Bottom-left corner of the triangle
//		Vertex2D v1 = { 250, 50, FLOAT_TO_FIXED(1.0f), FLOAT_TO_FIXED(1.0f) };  // Bottom-right corner of the triangle
//		Vertex2D v2 = { 150, 200, FLOAT_TO_FIXED(0.5f), FLOAT_TO_FIXED(0.0f) }; // Top-center of the texture
//
//		//DrawTexturedTriangle(v0, v1, v2, blueTexture, textureWidth, textureHeight);
//		DrawTri(exampleTriangle, SOLIDWHITE);
//
//		rot.y += 30;
//		rotFast.y += 30;
//
//		if (rot.y >= 360)
//		{
//			rot.y = 0;
//		}
//
//		if (rotFast.y >= 360)
//		{
//			rotFast.y = 0;
//		}
//
//		VPUWaitVSync();
//		CFLUSH_D_L1;
//		VPUSwapPages(&g_vx, &g_sc);
//
//	}
//}