/*
* Reading the ST-NICCC megademo data stored in
* the SDCard and streaming it to polygons,
* rendered in the framebuffer.
* 
* The polygon stream is a 640K file 
* (C_EXAMPLES/DATA/scene1.dat), that needs to 
* be stored on the SD card. 
*
* More details and links in C_EXAMPLES/DATA/notes.txt
*/

#include "basesystem.h"
#include "core.h"
#include "vpu.h"
#include "tru.h"
#include "dma.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cur_byte_address = 0;

FILE *s_fp;
uint8_t *filedata;
EVideoContext s_vctx;
ERasterizerContext s_rctx;
uint8_t *s_framebufferB;
uint8_t *s_framebufferA;
//uint8_t *s_rasterBuffer; // For hardware

static uint8_t next_byte(void)
{
	uint8_t result;
	// fread(&result, 1, 1, s_fp);
	result = filedata[cur_byte_address];
	++cur_byte_address;
	return result;
}

static uint16_t next_word(void)
{
/* In the ST-NICCC file,  
* words are stored in big endian format.
* (see DATA/scene_description.txt).
*/
	uint16_t hi = (uint16_t)next_byte();    
	uint16_t lo = (uint16_t)next_byte();
	return (hi << 8) | lo;
}

static inline void map_vertex(int16_t* X, int16_t* Y)
{
	*X = (*X*125)/100;
	*Y = (*Y*120)/100;
}

static void clear(void) {
	SPrimitive prim;
	prim.x0 = 0;
	prim.y0 = 0;
	prim.x1 = 319;
	prim.y1 = 239;
	prim.x2 = 319;
	prim.y2 = 0;
	TRUPushPrimitive(&s_rctx, &prim);
	TRUSetColor(&s_rctx, 0);
	TRURasterizePrimitive(&s_rctx);

	prim.x0 = 319;
	prim.y0 = 239;
	prim.x1 = 0;
	prim.y1 = 0;
	prim.x2 = 0;
	prim.y2 = 239;
	TRUPushPrimitive(&s_rctx, &prim);
	TRUSetColor(&s_rctx, 0);
	TRURasterizePrimitive(&s_rctx);
}

/*
* Current frame's vertices coordinates (if frame is indexed).
*/
int16_t  X[255];
int16_t  Y[255];

/*
* Current polygon vertices, lain out as
* xi = poly[2*i], yi = poly[2*i+1]
*/
int16_t   poly[30];

/*
* Masks for frame flags.
*/
#define CLEAR_BIT   1
#define PALETTE_BIT 2
#define INDEXED_BIT 4

/*
* Reads a frame's polygonal description from
* file and rasterizes the polygons using
* FemtoGL.
* returns 0 if last frame.
*   See DATA/scene_description.txt for the 
* ST-NICCC file format.
*   See DATA/test_ST_NICCC.c for an example
* program.
*/
static int read_frame(void)
{
	uint8_t frame_flags = next_byte();

	// Update palette data.
	if(frame_flags & PALETTE_BIT)
	{
		uint16_t colors = next_word();
		for(int b=15; b>=0; --b)
		{
			if(colors & (1 << b))
			{
				int rgb = next_word();
				// Get the three 3-bits per component R,G,B
				int b3 = (rgb & 0x007);
				int g3 = (rgb & 0x070) >> 4;
				int r3 = (rgb & 0x700) >> 8;

				// Set the actual hardware color register
				VPUSetPal(15-b, r3*2,g3*2,b3*2);
			}
		}
	}

	if(frame_flags & CLEAR_BIT)
		clear(); 

	// Update vertices
	if(frame_flags & INDEXED_BIT)
	{
		uint8_t nb_vertices = next_byte();
		for(int v=0; v<nb_vertices; ++v)
		{
			X[v] = next_byte();
			Y[v] = next_byte();
			map_vertex(&X[v],&Y[v]);
		}
	}

	// Draw frame's polygons
	for(;;)
	{
		uint8_t poly_desc = next_byte();

		// Special polygon codes (end of frame,
		// seek next block, end of stream)

		if(poly_desc == 0xff)
			break; // end of frame

		if(poly_desc == 0xfe)
		{
			// Go to next 64kb block
			// (TODO: with fseek !)
			while(cur_byte_address & 65535)
				next_byte();
			return 1; 
		}

		if(poly_desc == 0xfd)
			return 0; // end of stream

		uint8_t nvrtx = poly_desc & 15;
		uint8_t poly_col = poly_desc >> 4;
		for(int i=0; i<nvrtx; ++i)
		{
			if(frame_flags & INDEXED_BIT)
			{
				uint8_t index = next_byte();
				poly[2*i]   = X[index];
				poly[2*i+1] = Y[index];
			}
			else
			{
				int16_t x,y;
				x = next_byte();
				y = next_byte();
				map_vertex(&x,&y);
				poly[2*i]   = x;
				poly[2*i+1] = y;
			}
		}

		// Decompose into a triangle fan and rasterize
		for (int i=0;i<nvrtx-2;++i)
		{
			SPrimitive prim;
			prim.x0 = poly[2*0];
			prim.y0 = poly[2*0+1];
			prim.x1 = poly[2*(i+1)];
			prim.y1 = poly[2*(i+1)+1];
			prim.x2 = poly[2*(i+2)];
			prim.y2 = poly[2*(i+2)+1];

			TRUPushPrimitive(&s_rctx, &prim);
			TRUSetColor(&s_rctx, poly_col);
			TRURasterizePrimitive(&s_rctx);
		}
	}
	return 1; 
}

static int read_frame_flip(void)
{
	uint8_t frame_flags = next_byte();

	// Update palette data.
	if(frame_flags & PALETTE_BIT)
	{
		uint16_t colors = next_word();
		for(int b=15; b>=0; --b)
		{
			if(colors & (1 << b))
			{
				int rgb = next_word();
				// Get the three 3-bits per component R,G,B
				int b3 = (rgb & 0x007);
				int g3 = (rgb & 0x070) >> 4;
				int r3 = (rgb & 0x700) >> 8;

				// Set the actual hardware color register
				VPUSetPal(15-b, r3*2,g3*2,b3*2);
			}
		}
	}

	if(frame_flags & CLEAR_BIT)
		clear(); 

	// Update vertices
	if(frame_flags & INDEXED_BIT)
	{
		uint8_t nb_vertices = next_byte();
		for(int v=0; v<nb_vertices; ++v)
		{
			X[v] = next_byte();
			Y[v] = next_byte();
			map_vertex(&X[v],&Y[v]);
		}
	}

	// Draw frame's polygons
	for(;;)
	{
		uint8_t poly_desc = next_byte();

		// Special polygon codes (end of frame,
		// seek next block, end of stream)

		if(poly_desc == 0xff)
			break; // end of frame

		if(poly_desc == 0xfe)
		{
			// Go to next 64kb block
			// (TODO: with fseek !)
			while(cur_byte_address & 65535)
				next_byte();
			return 1; 
		}

		if(poly_desc == 0xfd)
			return 0; // end of stream

		uint8_t nvrtx = poly_desc & 15;
		uint8_t poly_col = poly_desc >> 4;
		for(int i=0; i<nvrtx; ++i)
		{
			if(frame_flags & INDEXED_BIT)
			{
				uint8_t index = next_byte();
				poly[2*i]   = X[index];
				poly[2*i+1] = Y[index];
			}
			else
			{
				int16_t x,y;
				x = next_byte();
				y = next_byte();
				map_vertex(&x,&y);
				poly[2*i]   = x;
				poly[2*i+1] = y;
			}
		}

		// Decompose into a triangle fan and rasterize
		for (int i=0;i<nvrtx-2;++i)
		{
			SPrimitive prim;
			prim.x0 = poly[2*0];
			prim.y0 = poly[2*0+1];
			prim.x2 = poly[2*(i+1)];
			prim.y2 = poly[2*(i+1)+1];
			prim.x1 = poly[2*(i+2)];
			prim.y1 = poly[2*(i+2)+1];

			TRUPushPrimitive(&s_rctx, &prim);
			TRUSetColor(&s_rctx, poly_col);
			TRURasterizePrimitive(&s_rctx);
		}
	}
	return 1; 
}

int main(int argc, char** argv)
{
	s_framebufferB = VPUAllocateBuffer(320*240); // Or think of it as 1280*64 for tiles
	s_framebufferA = VPUAllocateBuffer(320*240);
	//s_rasterBuffer = VPUAllocateBuffer(80*60*16); // Rasterizer tile buffer

	//memset(s_rasterBuffer, 0x07, 80*60*16);
	memset(s_framebufferA, 0x07, 320*240);
	memset(s_framebufferB, 0x07, 320*240);

	s_vctx.m_vmode = EVM_320_Wide;
	s_vctx.m_cmode = ECM_8bit_Indexed;
	VPUSetVMode(&s_vctx, EVS_Enable);
	VPUSetDefaultPalette(&s_vctx);

	struct EVideoSwapContext sc;
	sc.cycle = 0;
	sc.framebufferA = s_framebufferA;
	sc.framebufferB = s_framebufferB;
	VPUSwapPages(&s_vctx, &sc);

	//TRUSetTileBuffer(&s_rctx, (uint32_t)s_rasterBuffer); // For hardware
	TRUSetTileBuffer(&s_rctx, (uint32_t)sc.writepage);

	while(1)
	{
		cur_byte_address = 0;
		if (argc<=1)
			s_fp = fopen("scene1.dat", "rb");
		else
			s_fp = fopen(argv[1], "rb");
		if (!s_fp)
		{
			printf("Could not open scene data\n");
			return -1;
		}

		// Read the whole thing into memory
		{
			fpos_t pos, endpos;
			fgetpos(s_fp, &pos);
			fseek(s_fp, 0, SEEK_END);
			fgetpos(s_fp, &endpos);
			fsetpos(s_fp, &pos);
			uint32_t fsize = (uint32_t)endpos;
			filedata = (uint8_t*)malloc(fsize);
			fread(filedata, 1, fsize, s_fp);
			fclose(s_fp);
		}

		int res = 0;
		if (argc<=1)
		{
			do
			{
				res = read_frame();

				// Make sure to flush rasterizer cache to raster memory before it's read
				TRUFlushCache(&s_rctx);
				TRUInvalidateCache(&s_rctx);

				// Wait for all raster work to finish
				TRUBarrier(&s_rctx);
				TRUWait(&s_rctx);

				// TODO: Get the DMA unit to resolve and write output onto the current VPU write page
				//DMAResolveTiles((uint32_t)s_rasterBuffer, (uint32_t)sc.writepage);

				if (argc<=1) // no vsync if we have a command line argument
					VPUWaitVSync();
				VPUSwapPages(&s_vctx, &sc);
				TRUSetTileBuffer(&s_rctx, (uint32_t)sc.writepage);
			} while(res);
		}
		else
		{
			do
			{
				res = read_frame_flip();

				// Make sure to flush rasterizer cache to raster memory before it's read
				TRUFlushCache(&s_rctx);
				TRUInvalidateCache(&s_rctx);

				// Wait for all raster work to finish
				TRUBarrier(&s_rctx);
				TRUWait(&s_rctx);

				// TODO: Get the DMA unit to resolve and write output onto the current VPU write page
				//DMAResolveTiles((uint32_t)s_rasterBuffer, (uint32_t)sc.writepage);

				if (argc<=1) // no vsync if we have a command line argument
					VPUWaitVSync();
				VPUSwapPages(&s_vctx, &sc);
				TRUSetTileBuffer(&s_rctx, (uint32_t)sc.writepage);
			} while(res);
		}
	}
}
