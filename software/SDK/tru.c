/**
 * @file tru.c
 * 
 * @brief TRU rasterizer driver
 *
 * This file contains the software driver for the TRU rasterizer.
 *
 * The TRU rasterizer is a hardware rasterizer that is capable of rendering
 * triangles to a 320x240 framebuffer. The rasterizer is controlled by writing
 * commands to the TRUCMDIO register.
 * 
 * @note This code is work in progress and not yet functioning with high accuracy.
 */

#include "basesystem.h"
#include "tru.h"
#include "core.h"
#include <stdlib.h>

static inline int32_t gmax(int32_t x, int32_t y) { return x>=y?x:y; }
static inline int32_t gmin(int32_t x, int32_t y) { return x<=y?x:y; }

// Rasterizer control
#ifdef TRU_HARDWARE
volatile uint32_t *TRUCMDIO = (volatile uint32_t* ) DEVICE_TRUC;
#endif

#ifndef TRU_HARDWARE
/**
 * @brief Calculate edge function for a point
 * 
 * Calculate the edge function for a point relative to a line.
 * 
 * @param v0 First vertex of the line
 * @param v1 Second vertex of the line
 * @param A Pointer to the A coefficient
 * @param B Pointer to the B coefficient
 * @param p Point to calculate edge function for
 * @return Edge function value
 */
int32_t TRUEdgeFunction(const struct TRUVec2 *v0, const struct TRUVec2 *v1, int32_t *A, int32_t *B, const struct TRUVec2 *p)
{
	*A = v0->y - v1->y;
	*B = v1->x - v0->x;
	int32_t C = v0->x*v1->y - v0->y*v1->x;

	// Value at upper left corner of 4x4 tile
	// Add A to the following to advance left or B to advance down one pixel
	return (*A)*p->x + (*B)*p->y + C;
}
#endif

/**
 * @brief Set the rasterizer tile buffer address
 * 
 * Set the address of the rasterizer tile buffer.
 * 
 * @param _rc Rasterizer context
 * @param _TRUWriteAddress16ByteAligned 16-byte aligned address of the tile buffer
 */
void TRUSetTileBuffer(struct ERasterizerContext *_rc, const uint32_t _TRUWriteAddress16ByteAligned)
{
#ifndef TRU_HARDWARE
	_rc->m_rasterOutAddrressCacheAligned = _TRUWriteAddress16ByteAligned;
#else
	*TRUCMDIO = RASTERCMD_OUTADDRS;
	*TRUCMDIO = _TRUWriteAddress16ByteAligned;
#endif
}

/**
 * @brief Push a primitive to the rasterizer queue
 * 
 * Push a primitive to the rasterizer queue. The primitive is a triangle defined by
 * three vertices.
 * This function will calculate the bounding box of the triangle and generate edge
 * functions at the min corner of the bounding box.
 * The min corner of the bounding box is rounded to the nearest 4-pixel boundary
 * so that it aligns to a 4x4 tile.
 * 
 * @param _rc Rasterizer context
 * @param _primitive Primitive to push
 */
void TRUPushPrimitive(struct ERasterizerContext *_rc, struct SPrimitive* _primitive)
{
#ifndef TRU_HARDWARE
	_rc->v0.x = _primitive->x0;
	_rc->v0.y = _primitive->y0;
	_rc->v1.x = _primitive->x1;
	_rc->v1.y = _primitive->y1;
	_rc->v2.x = _primitive->x2;
	_rc->v2.y = _primitive->y2;

	// Generate AABB
	_rc->m_minx = gmin(_rc->v0.x, gmin(_rc->v1.x, _rc->v2.x));
	_rc->m_miny = gmin(_rc->v0.y, gmin(_rc->v1.y, _rc->v2.y));
	_rc->m_maxx = gmax(_rc->v0.x, gmax(_rc->v1.x, _rc->v2.x));
	_rc->m_maxy = gmax(_rc->v0.y, gmax(_rc->v1.y, _rc->v2.y));

	// Clip to viewport (TODO: assuming same as hardware here, 320x240)
	_rc->m_minx = gmax(0,gmin(319, _rc->m_minx));
	_rc->m_miny = gmax(0,gmin(239, _rc->m_miny));
	_rc->m_maxx = gmax(0,gmin(319, _rc->m_maxx));
	_rc->m_maxy = gmax(0,gmin(239, _rc->m_maxy));

	// Round x to multiples of 4 pixels
	_rc->m_minx = _rc->m_minx&0xFFFFFFFFC;
	_rc->m_maxx = (_rc->m_maxx+3)&0xFFFFFFFFC;

	// Generate edge functions for min AABB corner
	struct TRUVec2 p;
	p.x = _rc->m_minx;
	p.y = _rc->m_miny;

	_rc->w0_row = TRUEdgeFunction(&_rc->v1, &_rc->v2, &_rc->a12, &_rc->b12, &p);
	_rc->w1_row = TRUEdgeFunction(&_rc->v2, &_rc->v0, &_rc->a20, &_rc->b20, &p);
	_rc->w2_row = TRUEdgeFunction(&_rc->v0, &_rc->v1, &_rc->a01, &_rc->b01, &p);
#else
	*TRUCMDIO = RASTERCMD_PUSHVERTEX0;
	*TRUCMDIO = (uint32_t)(_primitive->y0<<16) | (((uint32_t)_primitive->x0)&0xFFFF);
	*TRUCMDIO = RASTERCMD_PUSHVERTEX1;
	*TRUCMDIO = (uint32_t)(_primitive->y1<<16) | (((uint32_t)_primitive->x1)&0xFFFF);
	*TRUCMDIO = RASTERCMD_PUSHVERTEX2;
	*TRUCMDIO = (uint32_t)(_primitive->y2<<16) | (((uint32_t)_primitive->x2)&0xFFFF);
#endif
}

/**
 * @brief Rasterize the primitive
 * 
 * Rasterize the primitive that was pushed to the rasterizer queue.
 * This function will sweep the triangle and fill the pixels that are inside the
 * primitive, one tile at a time.
 * 
 * It will first scan right to find the right edge of the primitive, then scan
 * left to find the left edge of the primitive. This way it can fill the pixels
 * without going outside the primitive too much and wasting cycles.
 * 
 * @param _rc Rasterizer context
 */
void TRURasterizePrimitive(struct ERasterizerContext *_rc)
{
#ifndef TRU_HARDWARE
	int32_t left = _rc->m_minx;
	int32_t right = _rc->m_maxx;

	int32_t w0 = _rc->w0_row;
	int32_t w1 = _rc->w1_row;
	int32_t w2 = _rc->w2_row;

	uint32_t *rasterOut = (uint32_t*)(_rc->m_rasterOutAddrressCacheAligned + _rc->m_miny*320);
	for (int32_t y = _rc->m_miny; y<=_rc->m_maxy; y+=2)
	{
		// Scan to right
		uint32_t prevMaskAcc = 0;
		for (int32_t x = left; x<=_rc->m_maxx; x+=4)
		{
			uint32_t val0 = ((w0&w1&w2)>>31) & 0x000000FF;
			w0 += _rc->a12;
			w1 += _rc->a20;
			w2 += _rc->a01;

			uint32_t val1 = ((w0&w1&w2)>>31) & 0x0000FF00;
			w0 += _rc->a12;
			w1 += _rc->a20;
			w2 += _rc->a01;

			uint32_t val2 = ((w0&w1&w2)>>31) & 0x00FF0000;
			w0 += _rc->a12;
			w1 += _rc->a20;
			w2 += _rc->a01;

			uint32_t val3 = ((w0&w1&w2)>>31) & 0xFF000000;
			w0 += _rc->a12;
			w1 += _rc->a20;
			w2 += _rc->a01;

			uint32_t wmask = (val0 | val1 | val2 | val3);
			right = x;
			if (!wmask && prevMaskAcc)
				break;
			prevMaskAcc |= wmask;

			if (wmask == 0xFFFFFFFF)
				rasterOut[x/4] = _rc->m_color & wmask;
			else if (wmask != 0x00000000)
			{
				uint32_t imask = ~wmask;
				rasterOut[x/4] = (rasterOut[x/4] & imask) | (_rc->m_color & wmask);
			}
		}

		// Next row
		w0 -= _rc->a12; // back one pixel
		w1 -= _rc->a20;
		w2 -= _rc->a01;
		w0 += _rc->b12; // down one pixel
		w1 += _rc->b20;
		w2 += _rc->b01;
		rasterOut += 80;

		// Scan to left
		prevMaskAcc = 0;
		for (int32_t x = right; x>=_rc->m_minx; x-=4)
		{
			uint32_t val0 = ((w0&w1&w2)>>31) & 0xFF000000;
			w0 -= _rc->a12;
			w1 -= _rc->a20;
			w2 -= _rc->a01;

			uint32_t val1 = ((w0&w1&w2)>>31) & 0x00FF0000;
			w0 -= _rc->a12;
			w1 -= _rc->a20;
			w2 -= _rc->a01;

			uint32_t val2 = ((w0&w1&w2)>>31) & 0x0000FF00;
			w0 -= _rc->a12;
			w1 -= _rc->a20;
			w2 -= _rc->a01;

			uint32_t val3 = ((w0&w1&w2)>>31) & 0x000000FF;
			w0 -= _rc->a12;
			w1 -= _rc->a20;
			w2 -= _rc->a01;

			uint32_t wmask = (val0 | val1 | val2 | val3);
			left = x;
			if (!wmask && prevMaskAcc)
				break;
			prevMaskAcc |= wmask;

			if (wmask == 0xFFFFFFFF)
				rasterOut[x/4] = _rc->m_color & wmask;
			else if (wmask != 0x00000000)
			{
				uint32_t imask = ~wmask;
				rasterOut[x/4] = (rasterOut[x/4] & imask) | (_rc->m_color & wmask);
			}
		}

		// Next row
		w0 += _rc->a12; // back one pixel
		w1 += _rc->a20;
		w2 += _rc->a01;
		w0 += _rc->b12; // down one pixel
		w1 += _rc->b20;
		w2 += _rc->b01;
		rasterOut += 80;
	}
#else
	*TRUCMDIO = RASTERCMD_RASTERIZEPRIM;
#endif
}

/**
 * @brief Set the primitive color
 * 
 * Set the color to be used when rasterizing the following primitives.
 * The color is an 8 bit index to a color palette, and is replicated to all bytes of a word.
 * This makes it possible to write the color to the framebuffer in a single write for 4 adjacent pixels.
 * @note Hardware rasterizer will use masked memory writes to avoid read-modify-write cycles and can emit all 4 pixels in a single write, hence the replication.
 * 
 * @param _rc Rasterizer context
 * @param _colorIndex Color index
 */
void TRUSetColor(struct ERasterizerContext *_rc, const uint8_t _colorIndex)
{
#ifndef TRU_HARDWARE
	_rc->m_color = (_colorIndex<<24) | (_colorIndex<<16) | (_colorIndex<<8) | _colorIndex;
#else
	*TRUCMDIO = RASTERCMD_SETRASTERCOLOR;
	*TRUCMDIO = (_colorIndex<<24) | (_colorIndex<<16) | (_colorIndex<<8) | _colorIndex;
#endif
}

/**
 * @brief Flush the rasterizer cache
 * 
 * Flush the rasterizer cache. This will write any pending data from rasterizer to system memory.
 * 
 * @param _rc Rasterizer context
 */
void TRUFlushCache(struct ERasterizerContext *_rc)
{
#ifndef TRU_HARDWARE
	CFLUSH_D_L1;
#else
	*TRUCMDIO = RASTERCMD_FLUSHCACHE;
#endif
}

/**
 * @brief Invalidate the rasterizer cache
 * 
 * Invalidate the rasterizer cache. This will discard any pending data from rasterizer.
 * 
 * @param _rc Rasterizer context
 */
void TRUInvalidateCache(struct ERasterizerContext *_rc)
{
#ifndef TRU_HARDWARE
	// TODO:
#else
	*TRUCMDIO = RASTERCMD_INVALIDATECACHE;
#endif
}

/**
 * @brief Rasterizer wait barrier
 * 
 * This will insert a barrier in the rasterizer command stream which the CPU can wait for.
 * 
 * @param _rc Rasterizer context
 */
void TRUBarrier(struct ERasterizerContext *_rc)
{
#ifndef TRU_HARDWARE
	// TODO:
#else
	*TRUCMDIO = RASTERCMD_BARRIER;
#endif
}

/**
 * @brief Check if there are pending commands in the rasterizer
 * 
 * Check if there are pending commands in the rasterizer.
 * 
 * @param _rc Rasterizer context
 * @return Number of pending commands
 * @see TRUWait
 */
uint32_t TRUPending(struct ERasterizerContext *_rc)
{
#ifndef TRU_HARDWARE
	return 0;
#else
	return *TRUCMDIO;
#endif
}

/**
 * @brief Wait for the rasterizer to finish
 * 
 * Wait for the rasterizer to finish.
 * 
 * @param _rc Rasterizer context
 * @see TRUPending
 */
void TRUWait(struct ERasterizerContext *_rc)
{
#ifndef TRU_HARDWARE
	// TODO:
#else
	while (*TRUCMDIO) { asm volatile("nop;"); }
#endif
}
