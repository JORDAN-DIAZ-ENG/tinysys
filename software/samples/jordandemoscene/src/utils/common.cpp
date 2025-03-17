#include "vpu.h"

#include "common.h"

#include <cmath> // For sin, cos functions, replace later with LUT
#include <algorithm> // For std::sort


static uint8_t* s_framebufferA = nullptr;
static uint8_t* s_framebufferB = nullptr;

struct EVideoContext     g_vx;
struct EVideoSwapContext g_sc;
EColorMode COLOR_MODE = ECM_8bit_Indexed;

void allocVideo()
{
	if (!s_framebufferA)
	{
		s_framebufferA = VPUAllocateBuffer(640 * 480);
		s_framebufferB = VPUAllocateBuffer(640 * 480);

		printf("ALLOC VIDEO %p %p\n", s_framebufferA, s_framebufferB);
	}
}

void set320x240(void)
{
	allocVideo();

	g_vx.m_vmode = EVM_320_Wide;
	g_vx.m_cmode = COLOR_MODE;
	VPUSetVMode(&g_vx, EVS_Enable);

	g_sc.cycle = 0;
	g_sc.framebufferA = s_framebufferA;
	g_sc.framebufferB = s_framebufferB;
	//CFLUSH_D_L1;
	VPUSwapPages(&g_vx, &g_sc);
}

void set640x480(void)
{
	allocVideo();

	g_vx.m_vmode = EVM_640_Wide;
	g_vx.m_cmode = COLOR_MODE;
	VPUSetVMode(&g_vx, EVS_Enable);

	g_sc.cycle = 0;
	g_sc.framebufferA = s_framebufferA;
	g_sc.framebufferB = s_framebufferB;
	//CFLUSH_D_L1;
	VPUSwapPages(&g_vx, &g_sc);
}

static uint8_t s_palette[ 256 ][ 3 ]; // 256 colors, 3 bytes per color (RGB)
static int s_brightness = 256;

void setPaletteAtIdx(const uint8_t idx, const uint8_t r, const uint8_t g, const uint8_t b)
{
	s_palette[idx][0] = r;
	s_palette[idx][1] = g;
	s_palette[idx][2] = b;

	VPUSetPal(idx, (r * s_brightness) >> 8, (g * s_brightness) >> 8, (b * s_brightness) >> 8);
}

void loadPalette(uint8_t* pal, const int startingIndex, const int paletteSize)
{
	for (int i = 0; i < paletteSize; ++i)
	{
		s_palette[i][0] = pal[i * 3];
		s_palette[i][1] = pal[i * 3 + 1];
		s_palette[i][2] = pal[i * 3 + 2];


		setPaletteAtIdx(i + startingIndex, pal[i * 3], pal[i * 3 + 1], pal[i * 3 + 2]);
	}
}

void tickPalette()
{
	for (int i = 0; i < 256; ++i)
	{
		VPUSetPal(i, (s_palette[i][0] * s_brightness) >> 8, (s_palette[i][1] * s_brightness) >> 8, (s_palette[i][2] * s_brightness) >> 8);
	}
}

void setPixel(int16_t x, int16_t y, uint32_t color)
{
	if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return; // Bounds check, negatives don't need to be checked because of unsigned coordinates

	g_sc.writepage[y * SCREEN_WIDTH + x] = color;
}

void DrawLineFast(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint32_t color)
{
	int16_t deltaX = abs(x1 - x0);
	int16_t deltaY = abs(y1 - y0);
	int16_t signX = (x0 < x1) ? 1 : -1;
	int16_t signY = (y0 < y1) ? 1 : -1;

	int16_t error = deltaX - deltaY;
	uint8_t* pixelPtr;

	while (true)
	{
		pixelPtr = &g_sc.writepage[y0 * SCREEN_WIDTH + x0];
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


//constexpr int FIXED_SCALE = 128;


constexpr const int SCREEN_HALF_WIDTH = SCREEN_WIDTH / 2;
constexpr const int SCREEN_HALF_HEIGHT = SCREEN_HEIGHT / 2;

//void initTrigLUT()
//{
//	for (int i = 0; i < NUM_ANGLES; i++)
//	{
//		// Calculate sin and cos and store in LUT
//		sinLUT[i] = (int16_t)(sin(i * 3.14f / 180.0) * FIXED_SCALE_FAST);
//		cosLUT[i] = (int16_t)(cos(i * 3.14f / 180.0) * FIXED_SCALE_FAST);
//	}
//}
//
//// Fast sine function using LUT
//int16_t FixedSinFast(int16_t angle)
//{
//	// Normalize the angle to be in the range [0, 359]
//	angle = angle % 360;
//	if (angle < 0) angle += 360;
//
//	return sinLUT[angle];
//}
//
//// Fast cosine function using LUT
//int16_t FixedCosFast(int16_t angle)
//{
//	// Normalize the angle to be in the range [0, 359]
//	angle = angle % 360;
//	if (angle < 0) angle += 360;
//
//	return cosLUT[angle];
//}

//vec3 RotateYFast(vec3 v, int16_t angle)
//{
//	int16_t cosA = FixedCosFast(angle);
//	int16_t sinA = FixedSinFast(angle);
//
//	int16_t x = (v.x * cosA + v.z * sinA) / FIXED_SCALE_FAST;
//	int16_t z = (-v.x * sinA + v.z * cosA) / FIXED_SCALE_FAST;
//	return { x, v.y, z };
//}

uint32_t GetTextureColor(uint8_t* texture, int textureWidth, int textureHeight, int u, int v)
{
	u = u % textureWidth;  // Wrap around
	v = v % textureHeight; // Wrap around
	return texture[v * textureWidth + u]; // Assuming 8-bit indexed texture
}

// Fixed-Point Edge Function
int EdgeFunction(Vertex2D v0, Vertex2D v1, int x, int y)
{
	return (x - v0.x) * (v1.y - v0.y) - (y - v0.y) * (v1.x - v0.x);
}

void DrawTexturedTriangle(Vertex2D v0, Vertex2D v1, Vertex2D v2, uint8_t* texture, int textureWidth, int textureHeight)
{
	// Sort vertices by y-coordinate (ascending order)
	if (v1.y < v0.y) std::swap(v0, v1);
	if (v2.y < v0.y) std::swap(v0, v2);
	if (v2.y < v1.y) std::swap(v1, v2);

	// Bounding box
	int minX = std::min(std::min(v0.x, v1.x), v2.x);
	int minY = std::min(std::min(v0.y, v1.y), v2.y);
	int maxX = std::max(std::max(v0.x, v1.x), v2.x);
	int maxY = std::max(std::max(v0.y, v1.y), v2.y);

	// Loop over bounding box
	for (int y = minY; y <= maxY; y++)
	{
		for (int x = minX; x <= maxX; x++)
		{
			// Compute fixed-point barycentric coordinates
			int w0 = EdgeFunction(v1, v2, x, y);
			int w1 = EdgeFunction(v2, v0, x, y);
			int w2 = EdgeFunction(v0, v1, x, y);
			int sumW = w0 + w1 + w2;

			if (sumW > 0 && w0 >= 0 && w1 >= 0 && w2 >= 0)
			{
				// Normalize barycentric weights
				int w0_fixed = FIXED_DIV(w0, sumW);
				int w1_fixed = FIXED_DIV(w1, sumW);
				int w2_fixed = FIXED_DIV(w2, sumW);

				// Interpolate UV coordinates
				int u = FIXED_MUL(w0_fixed, v0.u) + FIXED_MUL(w1_fixed, v1.u) + FIXED_MUL(w2_fixed, v2.u);
				int v = FIXED_MUL(w0_fixed, v0.v) + FIXED_MUL(w1_fixed, v1.v) + FIXED_MUL(w2_fixed, v2.v);

				// Convert fixed-point UV to integer
				int texX = (FIXED_TO_INT(u) % textureWidth + textureWidth) % textureWidth;
				int texY = (FIXED_TO_INT(v) % textureHeight + textureHeight) % textureHeight;

				// Sample the texture
				uint8_t texColor = texture[texY * textureWidth + texX];

				// Draw the pixel
				setPixel(static_cast<int16_t>(x), static_cast<int16_t>(y), texColor);
			}
		}
	}
}

void DrawTri(const Tri& triangle, const uint32_t color)
{
	// Sort vertices by y-coordinate using an array
	vec3 v[3] = { triangle.v0, triangle.v1, triangle.v2 };
	std::sort(v, v + 3, [](const vec3& a, const vec3& b) { return a.y < b.y; });

	// Fixed-point factor (e.g., 256 gives an 8-bit fractional part)
	const int FIXED_FACTOR = 256;

	// Compute y differences for the long edge and for the top/bottom halves
	int dy_long = v[2].y - v[0].y;
	int dy_top = v[1].y - v[0].y;
	int dy_bot = v[2].y - v[1].y;

	// Precompute fixed-point dx/dy for each edge.
	// For the long edge (v0 -> v2)
	int dx_long = (dy_long > 0) ? ((v[2].x - v[0].x) * FIXED_FACTOR) / dy_long : 0;
	int x_long = v[0].x * FIXED_FACTOR;

	// For the top edge (v0 -> v1)
	int dx_top = (dy_top > 0) ? ((v[1].x - v[0].x) * FIXED_FACTOR) / dy_top : 0;
	int x_top = v[0].x * FIXED_FACTOR;

	// For the bottom edge (v1 -> v2)
	int dx_bot = (dy_bot > 0) ? ((v[2].x - v[1].x) * FIXED_FACTOR) / dy_bot : 0;
	int x_bot = v[1].x * FIXED_FACTOR;

	// Process the top half: from y = v[0].y to y = v[1].y (inclusive)
	for (int y = v[0].y; y <= v[1].y; y++)
	{
		// Convert fixed-point positions to integer pixels (using rounding)
		int ix_long = (x_long + FIXED_FACTOR / 2) / FIXED_FACTOR;
		int ix_top = (x_top + FIXED_FACTOR / 2) / FIXED_FACTOR;
		int leftX = std::min(ix_long, ix_top);
		int rightX = std::max(ix_long, ix_top);

		for (int x = leftX; x <= rightX; x++)
		{
			setPixel(x, y, color);
		}

		// Increment the fixed-point x positions for each edge
		x_long += dx_long;
		x_top += dx_top;
	}

	// Process the bottom half: from y = v[1].y+1 to y = v[2].y (inclusive)
	for (int y = v[1].y + 1; y <= v[2].y; y++)
	{
		int ix_long = (x_long + FIXED_FACTOR / 2) / FIXED_FACTOR;
		int ix_bot = (x_bot + FIXED_FACTOR / 2) / FIXED_FACTOR;
		int leftX = std::min(ix_long, ix_bot);
		int rightX = std::max(ix_long, ix_bot);

		for (int x = leftX; x <= rightX; x++)
		{
			setPixel(x, y, color);
		}

		// Increment the fixed-point x positions for each edge
		x_long += dx_long;
		x_bot += dx_bot;
	}
}



void DrawWireframeFast(vec3* vertices, int* indices, int indexCount, uint32_t color, uint16_t scale, rotation& rot)
{
	for (int i = 0; i < indexCount; i += 3)
	{
		int index0 = indices[i];
		int index1 = indices[i + 1];
		int index2 = indices[i + 2];

		vec3 v0 = { vertices[index0].x / scale, vertices[index0].y / scale, vertices[index0].z / scale };
		vec3 v1 = { vertices[index1].x / scale, vertices[index1].y / scale, vertices[index1].z / scale };
		vec3 v2 = { vertices[index2].x / scale, vertices[index2].y / scale, vertices[index2].z / scale };

		//// Apply rotation in X, then Y, then Z
		////v0 = RotateX(v0, rotationX);
		//v0 = RotateYFast(v0, rot.y);
		////v0 = RotateZ(v0, rotationZ);

		////v1 = RotateX(v1, rotationX);
		//v1 = RotateYFast(v1, rot.y);
		////v1 = RotateZ(v1, rotationZ);

		////v2 = RotateX(v2, rotationX);
		//v2 = RotateYFast(v2, rot.y);
		////v2 = RotateZ(v2, rotationZ);

		// Projection to screen
		vec3 p0 = { SCREEN_HALF_WIDTH + v0.x, SCREEN_HALF_HEIGHT - v0.y };
		vec3 p1 = { SCREEN_HALF_WIDTH + v1.x, SCREEN_HALF_HEIGHT - v1.y };
		vec3 p2 = { SCREEN_HALF_WIDTH + v2.x, SCREEN_HALF_HEIGHT - v2.y };

		DrawLineFast(p0.x, p0.y, p1.x, p1.y, color);
		DrawLineFast(p1.x, p1.y, p2.x, p2.y, color);
		DrawLineFast(p2.x, p2.y, p0.x, p0.y, color);

	}
}

void DrawTexturedMesh()
{

}
