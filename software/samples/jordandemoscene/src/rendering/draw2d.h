// draw2d.h - 2D Rendering API
#ifndef DRAW2D_H
#define DRAW2D_H

#include <stdint.h>
#include <vpu.h>

// Drawing Functions
void Render_SetDrawTarget(EVideoContext* context, uint8_t* buffer);
void Render_SetHBlankEffect(void (*handler)(), int scanline);

// Effects
void Render_InitScanlineEffect();
void Render_PlasmaEffect();
void Render_FireEffect();
void Render_GradientEffect();

// Sprite Rendering
void Render_DrawSprite(int x, int y, const uint8_t* sprite, int width, int height, uint8_t transparentColor, uint8_t paletteOffset);
void Render_BlitSprite(int x, int y, const uint8_t* sprite, int width, int height); // No transparency, but faster

// Lines
void Render_DrawLine(int x0, int y0, int x1, int y1, uint8_t color);

#endif // DRAW2D_H