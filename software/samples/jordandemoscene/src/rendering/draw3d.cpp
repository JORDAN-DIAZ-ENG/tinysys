#include "draw3d.h"
#include "draw2d.h"

#include "jmath.h"
#include <vpu.h>
#include <algorithm>
#include <unordered_set>  // Required for std::unordered_set

#include "colors.h"
#include "maxwell.inl"

Vec3 ProjectVertex(Vec3 point)
{
	// Translate to camera space
	float camX = point.x - s_camera.position.x;
	float camY = point.y - s_camera.position.y;
	float camZ = point.z - s_camera.position.z;

	// Prevent division by zero (avoid projection issues)
	if (camZ < s_camera.nearClip) camZ = s_camera.nearClip;

	// Perspective divide
	float projectedX = (camX / camZ) * s_camera.fov;
	float projectedY = (camY / camZ) * s_camera.fov;

	// Convert to screen space
	Vec3 projected = { SCREEN_HALF_WIDTH + projectedX, SCREEN_HALF_HEIGHT - projectedY, camZ };
	return projected;
}

bool IsBackFacing(Vec3 p0, Vec3 p1, Vec3 p2)
{
	constexpr float EPSILON = 1e-6f;  // Small tolerance to prevent floating-point artifacts

	// Compute triangle normal using cross product
	Vec3 edge1 = { p1.x - p0.x, p1.y - p0.y, p1.z - p0.z };
	Vec3 edge2 = { p2.x - p0.x, p2.y - p0.y, p2.z - p0.z };

	Vec3 normal = {
		(edge1.y * edge2.z - edge1.z * edge2.y),
		(edge1.z * edge2.x - edge1.x * edge2.z),
		(edge1.x * edge2.y - edge1.y * edge2.x)
	};

	// View direction from camera to the triangle (subtract camera position)
	Vec3 viewDir = {
		p0.x - s_camera.position.x,
		p0.y - s_camera.position.y,
		p0.z - s_camera.position.z
	};

	// Dot product (normal dot view direction)
	float dot = (normal.x * viewDir.x + normal.y * viewDir.y + normal.z * viewDir.z);

	// Use epsilon threshold to prevent small numerical errors from causing incorrect culling
	return dot < -EPSILON;
}




/**
 * @brief Transforms a 3D point using a perspective projection (Fixed-Point)
 *        Uses Right-Handed coordinate system
 * @param point 3D point to project
 * @param fov Field of view scaling factor (fixed-point)
 * @return 2D screen coordinates
 */
Vec3 ProjectPoint(const Vec3& point, int fov)
{
	// Avoid division by zero (or negative z causing artifacts)
	if (point.z <= 0) return { 0, 0, -1 };

	int scale = FixedMul(fov, (1 << FIXED_POINT_SHIFT) / (fov + point.z));

	return {
		(FixedMul(point.x, scale) >> FIXED_POINT_SHIFT) + (SCREEN_WIDTH / 2),
		(FixedMul(point.y, scale) >> FIXED_POINT_SHIFT) + (SCREEN_HEIGHT / 2),
		point.z
	};
}

/**
 * @brief Draws a 3D line between two points
 * @param p1 First 3D point
 * @param p2 Second 3D point
 * @param color Line color
 */
void Render_Draw3DLine(const Vec3& p1, const Vec3& p2, uint8_t color)
{
	Vec3 proj1 = ProjectPoint(p1, DEFAULT_FOV);
	Vec3 proj2 = ProjectPoint(p2, DEFAULT_FOV);

	int x0 = proj1.x >> FIXED_POINT_SHIFT;
	int y0 = proj1.y >> FIXED_POINT_SHIFT;
	int x1 = proj2.x >> FIXED_POINT_SHIFT;
	int y1 = proj2.y >> FIXED_POINT_SHIFT;

	Render_DrawLine(x0, y0, x1, y1, color);
}

void Render_WireFrame(Vec3* vertices, int* indices, int indexCount, uint8_t color, uint16_t scale, EulerRot& rot)
{
	for (int i = 0; i < indexCount; i += 3)
	{
		int index0 = indices[i];
		int index1 = indices[i + 1];
		int index2 = indices[i + 2];

		Vec3 v0 = { vertices[index0].x / scale, vertices[index0].y / scale, vertices[index0].z / scale };
		Vec3 v1 = { vertices[index1].x / scale, vertices[index1].y / scale, vertices[index1].z / scale };
		Vec3 v2 = { vertices[index2].x / scale, vertices[index2].y / scale, vertices[index2].z / scale };

		v0 = Math_ApplyRotation(v0, rot);
		v1 = Math_ApplyRotation(v1, rot);
		v2 = Math_ApplyRotation(v2, rot);

		// Projection to screen
		//Vec3 p0 = { SCREEN_HALF_WIDTH + v0.x, SCREEN_HALF_HEIGHT - v0.y };
		//Vec3 p1 = { SCREEN_HALF_WIDTH + v1.x, SCREEN_HALF_HEIGHT - v1.y };
		//Vec3 p2 = { SCREEN_HALF_WIDTH + v2.x, SCREEN_HALF_HEIGHT - v2.y };
		Vec3 p0 = ProjectVertex(v0);
		Vec3 p1 = ProjectVertex(v1);
		Vec3 p2 = ProjectVertex(v2);

		// Wire frame
		Render_DrawLine(p0.x, p0.y, p1.x, p1.y, color);
		Render_DrawLine(p1.x, p1.y, p2.x, p2.y, color);
		Render_DrawLine(p2.x, p2.y, p0.x, p0.y, color);
	}
}

void Render_FilledMesh(Vec3* vertices, int* indices, int indexCount, uint8_t color, uint16_t scale, EulerRot& rot)
{
	//bool isColor = false;
	for (int i = 0; i < indexCount; i += 3)
	{
		int index0 = indices[i];
		int index1 = indices[i + 1];
		int index2 = indices[i + 2];

		Vec3 v0 = { vertices[index0].x / scale, vertices[index0].y / scale, vertices[index0].z / scale };
		Vec3 v1 = { vertices[index1].x / scale, vertices[index1].y / scale, vertices[index1].z / scale };
		Vec3 v2 = { vertices[index2].x / scale, vertices[index2].y / scale, vertices[index2].z / scale };

		v0 = Math_ApplyRotation(v0, rot);
		v1 = Math_ApplyRotation(v1, rot);
		v2 = Math_ApplyRotation(v2, rot);

		// Projection to screen
		Vec3 p0 = { SCREEN_HALF_WIDTH + v0.x, SCREEN_HALF_HEIGHT - v0.y };
		Vec3 p1 = { SCREEN_HALF_WIDTH + v1.x, SCREEN_HALF_HEIGHT - v1.y };
		Vec3 p2 = { SCREEN_HALF_WIDTH + v2.x, SCREEN_HALF_HEIGHT - v2.y };

		// Checkerboard colors
		//if (isColor)
		//{
		//	Render_FillTriangle(p0, p1, p2, color);
		//	isColor = false;
		//}
		//else
		//{
		//	Render_FillTriangle(p0, p1, p2, SOLIDPURPLE);
		//	isColor = true;
		//}

		// Solid Color
		Render_FillTriangle(p0, p2, p1, color);
	}
}

void Render_TexturedMesh(Vec3* vertices, UV* uvs, Vec3* normals, int* indices, int indexCount, uint8_t* texture, int texWidth, int texHeight, uint16_t scale, EulerRot& rot, uint8_t paletteOffset)
{
	for (int i = 0; i < indexCount; i += 3)
	{
		int index0 = indices[i];
		int index1 = indices[i + 1];
		int index2 = indices[i + 2];

		// Apply scaling
		Vec3 v0 = { vertices[index0].x / scale, vertices[index0].y / scale, vertices[index0].z / scale };
		Vec3 v1 = { vertices[index1].x / scale, vertices[index1].y / scale, vertices[index1].z / scale };
		Vec3 v2 = { vertices[index2].x / scale, vertices[index2].y / scale, vertices[index2].z / scale };

		// Fetch UV coordinates
		UV uv0 = uvs[index0];
		UV uv1 = uvs[index1];
		UV uv2 = uvs[index2];

		// Project to screen
		v0 = Math_ApplyRotation(v0, rot);
		v1 = Math_ApplyRotation(v1, rot);
		v2 = Math_ApplyRotation(v2, rot);

		// Projection to screen
		Vec3 p0 = ProjectVertex(v0);
		Vec3 p1 = ProjectVertex(v1);
		Vec3 p2 = ProjectVertex(v2);

		// Pass UVs to `Render_TextureTriangle`
		Render_TextureTriangle(p0, p1, p2, uv0, uv1, uv2, normals, texture, texWidth, texHeight, paletteOffset);
	}
}

void Render_Maxwell(Vec3* vertices, UV* uvs, Vec3* normals, int* indices, int indexCount, uint8_t* texture, int texWidth, int texHeight, uint16_t scale, EulerRot& rot, uint8_t paletteOffset, Vec3 position)
{
	int Whiskers_indices[18] = {
		340, 341, 342,// 449
		343, 344, 345,
		346, 347, 348,
		349, 350, 351,
		352, 353, 354,
		360, 361, 362,
	};

	int whisker_indexes[1] = {449};

	// Convert Whiskers_indices to a hash set for fast lookup
	std::unordered_set<int> whiskerSet(Whiskers_indices, Whiskers_indices + 18);

	for (int i = 0; i < indexCount; i += 3)
	{
		int index0 = indices[i];
		int index1 = indices[i + 1];
		int index2 = indices[i + 2];

		// Apply scaling
		Vec3 v0 = { (vertices[index0].x / scale), (vertices[index0].y / scale), (vertices[index0].z / scale) };
		Vec3 v1 = { (vertices[index1].x / scale), (vertices[index1].y / scale), (vertices[index1].z / scale) };
		Vec3 v2 = { (vertices[index2].x / scale), (vertices[index2].y / scale), (vertices[index2].z / scale) };


		// Fetch UV coordinates
		UV uv0 = uvs[index0];
		UV uv1 = uvs[index1];
		UV uv2 = uvs[index2];

		// Project to screen
		v0 = Math_ApplyRotation(v0, rot);
		v1 = Math_ApplyRotation(v1, rot);
		v2 = Math_ApplyRotation(v2, rot);

		// Apply translation AFTER rotation
		v0.x += position.x;
		v0.y += position.y;
		v0.z += position.z;

		v1.x += position.x;
		v1.y += position.y;
		v1.z += position.z;

		v2.x += position.x;
		v2.y += position.y;
		v2.z += position.z;

		// Projection to screen
		Vec3 p0 = ProjectVertex(v0);
		Vec3 p1 = ProjectVertex(v1);
		Vec3 p2 = ProjectVertex(v2);


		// **Skip this triangle if any of its indices are in the whiskers set**
		if (whiskerSet.count(index0) || whiskerSet.count(index1) || whiskerSet.count(index2))
		{
			// Pass UVs to `Render_TextureTriangle`
			Render_MaxwellTextureTriangle(p0, p1, p2, uv0, uv1, uv2, normals, s_maxwellWhiskerImg, s_maxwellWhiskersSX, s_maxwellWhiskersSY, 65);
			continue;
		}

		// Pass UVs to `Render_TextureTriangle`
		Render_MaxwellTextureTriangle(p0, p1, p2, uv0, uv1, uv2, normals, texture, texWidth, texHeight, paletteOffset);
	}

}


void Render_FillTriangle(Vec3 p0, Vec3 p1, Vec3 p2, uint8_t color)
{
	// Sort vertices by Y-coordinate (p0 at top, p1 middle, p2 bottom)
	if (p0.y > p1.y) std::swap(p0, p1);
	if (p0.y > p2.y) std::swap(p0, p2);
	if (p1.y > p2.y) std::swap(p1, p2);

	// Compute edge slopes
	int dx1 = (p1.y - p0.y) ? ((p1.x - p0.x) << FIXED_POINT_SHIFT) / (p1.y - p0.y) : 0;
	int dx2 = (p2.y - p0.y) ? ((p2.x - p0.x) << FIXED_POINT_SHIFT) / (p2.y - p0.y) : 0;
	int dx3 = (p2.y - p1.y) ? ((p2.x - p1.x) << FIXED_POINT_SHIFT) / (p2.y - p1.y) : 0;

	int x1 = p0.x << FIXED_POINT_SHIFT;
	int x2 = x1;

	// Draw upper part of triangle
	for (int y = p0.y; y < p1.y; y++)
	{
		Render_DrawLine(x1 >> FIXED_POINT_SHIFT, y, x2 >> FIXED_POINT_SHIFT, y, color);
		x1 += dx1;
		x2 += dx2;
	}

	// Draw lower part of triangle
	x1 = p1.x << FIXED_POINT_SHIFT;
	for (int y = p1.y; y < p2.y; y++)
	{
		Render_DrawLine(x1 >> FIXED_POINT_SHIFT, y, x2 >> FIXED_POINT_SHIFT, y, color);
		x1 += dx3;
		x2 += dx2;
	}
}

#define UV_SCALE (1 << 3)  // Equivalent to dividing by 8

void Render_TextureScanline(int x1, int x2, float u1, float u2, float v1, float v2, int y, uint8_t* texture, int texWidth, int texHeight, uint8_t paletteOffset)
{
	// Ensure left-to-right order
	if (x1 > x2) { std::swap(x1, x2); std::swap(u1, u2); std::swap(v1, v2); }

	// Convert UVs to fixed-point for calculations
	int U1_fixed = int(u1 * (1 << FIXED_POINT_SHIFT));
	int U2_fixed = int(u2 * (1 << FIXED_POINT_SHIFT));
	int V1_fixed = int(v1 * (1 << FIXED_POINT_SHIFT));
	int V2_fixed = int(v2 * (1 << FIXED_POINT_SHIFT));

	// Compute step values
	int dx = x2 - x1;
	int du = dx ? (U2_fixed - U1_fixed) / dx : 0;
	int dv = dx ? (V2_fixed - V1_fixed) / dx : 0;

	int u = U1_fixed;
	int v = V1_fixed;

	// Iterate over the scanline
	for (int x = x1; x <= x2; x++)
	{
		// Scale UVs back to integers for texture lookup
		int texU = ((u >> FIXED_POINT_SHIFT) % texWidth);
		int texV = ((v >> FIXED_POINT_SHIFT) % texHeight);

		// Handle negative UVs properly
		if (texU < 0) texU += texWidth;
		if (texV < 0) texV += texHeight;

		// Sample texture
		uint8_t texColor = texture[texV * texWidth + texU];

		// Apply palette offset if the color is not transparent
		if (texColor != 0xFF)
		{
			g_swapContext.writepage[y * SCREEN_WIDTH + x] = texColor + paletteOffset;
		}

		// Advance UVs
		u += du;
		v += dv;
	}
}




void Render_TextureTriangle(Vec3 p0, Vec3 p1, Vec3 p2, UV uv0, UV uv1, UV uv2, Vec3 *normals, uint8_t* texture, int texWidth, int texHeight, uint8_t paletteOffset)
{
	// Back face culling
	if (IsBackFacing(p0, p1, p2))
		return;  // Skip rendering this triangle

	// Sort vertices by Y-coordinate
	if (p0.y > p1.y) { std::swap(p0, p1); std::swap(uv0, uv1); }
	if (p0.y > p2.y) { std::swap(p0, p2); std::swap(uv0, uv2); }
	if (p1.y > p2.y) { std::swap(p1, p2); std::swap(uv1, uv2); }

	// Extract sorted vertex positions
	int x_start = p0.x, y_start = p0.y;
	int x_middle = p1.x, y_middle = p1.y;
	int x_stop = p2.x, y_stop = p2.y;

	// Extract UVs
	float u_start = uv0.u, v_start = uv0.v;
	float u_middle = uv1.u, v_middle = uv1.v;
	float u_stop = uv2.u, v_stop = uv2.v;

	// Constants
	//constexpr float UV_SCALE_FACTOR = 1.f / 4096.f;
	constexpr float UV_SCALE_FACTOR = 1.f;
	constexpr float EPSILON = 1e-6f;

	// Precompute slopes
	int x_slope_1 = ((x_stop - x_start) << FIXED_POINT_SHIFT) / ((y_stop - y_start) + EPSILON);
	int x_slope_2 = ((x_middle - x_start) << FIXED_POINT_SHIFT) / ((y_middle - y_start) + EPSILON);
	int x_slope_3 = ((x_stop - x_middle) << FIXED_POINT_SHIFT) / ((y_stop - y_middle) + EPSILON);

	float uv_slope_1_u = (u_stop - u_start) / ((y_stop - y_start) + EPSILON);
	float uv_slope_1_v = (v_stop - v_start) / ((y_stop - y_start) + EPSILON);

	float uv_slope_2_u = (u_middle - u_start) / ((y_middle - y_start) + EPSILON);
	float uv_slope_2_v = (v_middle - v_start) / ((y_middle - y_start) + EPSILON);

	float uv_slope_3_u = (u_stop - u_middle) / ((y_stop - y_middle) + EPSILON);
	float uv_slope_3_v = (v_stop - v_middle) / ((y_stop - y_middle) + EPSILON);

	// Scanline iteration
	for (int y = y_start; y < y_stop; y++)
	{
		int x1 = x_start + (((y - y_start) * x_slope_1) >> FIXED_POINT_SHIFT);
		int x2;
		float u1 = u_start + (y - y_start) * uv_slope_1_u;
		float v1 = v_start + (y - y_start) * uv_slope_1_v;
		float u2, v2;

		if (y < y_middle)
		{
			x2 = x_start + (((y - y_start) * x_slope_2) >> FIXED_POINT_SHIFT);
			u2 = u_start + (y - y_start) * uv_slope_2_u;
			v2 = v_start + (y - y_start) * uv_slope_2_v;
		}
		else
		{
			x2 = x_middle + (((y - y_middle) * x_slope_3) >> FIXED_POINT_SHIFT);
			u2 = u_middle + (y - y_middle) * uv_slope_3_u;
			v2 = v_middle + (y - y_middle) * uv_slope_3_v;
		}

		// Ensure left-to-right order
		if (x1 > x2)
		{
			std::swap(x1, x2);
			std::swap(u1, u2);
			std::swap(v1, v2);
		}

		// Compute UV slope for scanline
		float uv_slope_u = (u2 - u1) / ((x2 - x1) + EPSILON);
		float uv_slope_v = (v2 - v1) / ((x2 - x1) + EPSILON);

		// Iterate over scanline
		for (int x = x1; x < x2; x++)
		{
			float u = (u1 + (x - x1) * uv_slope_u) * UV_SCALE_FACTOR;
			float v = (v1 + (x - x1) * uv_slope_v) * UV_SCALE_FACTOR;

			// Convert UVs to texture coordinates
			int texU = static_cast<int>(u * texWidth) & (texWidth - 1);
			int texV = static_cast<int>(v * texHeight) & (texHeight - 1);

			// Sample texture color
			uint8_t texColor = texture[texV * texWidth + texU];

			// Apply color if not transparent
			if (texColor != 0xFF)
			{
				g_swapContext.writepage[y * SCREEN_WIDTH + x] = texColor + paletteOffset;
			}
		}
	}
}



void Render_MaxwellTextureTriangle(Vec3 p0, Vec3 p1, Vec3 p2, UV uv0, UV uv1, UV uv2, Vec3* normals, uint8_t* texture, int texWidth, int texHeight, uint8_t paletteOffset)
{
	// Back face culling
	if (IsBackFacing(p0, p1, p2))
		return;  // Skip rendering this triangle

	// Sort vertices by Y-coordinate
	if (p0.y > p1.y) { std::swap(p0, p1); std::swap(uv0, uv1); }
	if (p0.y > p2.y) { std::swap(p0, p2); std::swap(uv0, uv2); }
	if (p1.y > p2.y) { std::swap(p1, p2); std::swap(uv1, uv2); }

	// Extract sorted vertex positions
	int x_start = p0.x, y_start = p0.y;
	int x_middle = p1.x, y_middle = p1.y;
	int x_stop = p2.x, y_stop = p2.y;

	// Extract UVs
	float u_start = uv0.u, v_start = uv0.v;
	float u_middle = uv1.u, v_middle = uv1.v;
	float u_stop = uv2.u, v_stop = uv2.v;

	// Constants
	//constexpr float UV_SCALE_FACTOR = 1.f / 4096.f;
	constexpr float UV_SCALE_FACTOR = 1.f;
	constexpr float EPSILON = 1e-6f;

	// Precompute slopes
	int x_slope_1 = ((x_stop - x_start) << FIXED_POINT_SHIFT) / ((y_stop - y_start) + EPSILON);
	int x_slope_2 = ((x_middle - x_start) << FIXED_POINT_SHIFT) / ((y_middle - y_start) + EPSILON);
	int x_slope_3 = ((x_stop - x_middle) << FIXED_POINT_SHIFT) / ((y_stop - y_middle) + EPSILON);

	float uv_slope_1_u = (u_stop - u_start) / ((y_stop - y_start) + EPSILON);
	float uv_slope_1_v = (v_stop - v_start) / ((y_stop - y_start) + EPSILON);

	float uv_slope_2_u = (u_middle - u_start) / ((y_middle - y_start) + EPSILON);
	float uv_slope_2_v = (v_middle - v_start) / ((y_middle - y_start) + EPSILON);

	float uv_slope_3_u = (u_stop - u_middle) / ((y_stop - y_middle) + EPSILON);
	float uv_slope_3_v = (v_stop - v_middle) / ((y_stop - y_middle) + EPSILON);

	// Scanline iteration
	for (int y = y_start; y < y_stop; y++)
	{
		int x1 = x_start + (((y - y_start) * x_slope_1) >> FIXED_POINT_SHIFT);
		int x2;
		float u1 = u_start + (y - y_start) * uv_slope_1_u;
		float v1 = v_start + (y - y_start) * uv_slope_1_v;
		float u2, v2;

		if (y < y_middle)
		{
			x2 = x_start + (((y - y_start) * x_slope_2) >> FIXED_POINT_SHIFT);
			u2 = u_start + (y - y_start) * uv_slope_2_u;
			v2 = v_start + (y - y_start) * uv_slope_2_v;
		}
		else
		{
			x2 = x_middle + (((y - y_middle) * x_slope_3) >> FIXED_POINT_SHIFT);
			u2 = u_middle + (y - y_middle) * uv_slope_3_u;
			v2 = v_middle + (y - y_middle) * uv_slope_3_v;
		}

		// Ensure left-to-right order
		if (x1 > x2)
		{
			std::swap(x1, x2);
			std::swap(u1, u2);
			std::swap(v1, v2);
		}

		// Compute UV slope for scanline
		float uv_slope_u = (u2 - u1) / ((x2 - x1) + EPSILON);
		float uv_slope_v = (v2 - v1) / ((x2 - x1) + EPSILON);

		// Iterate over scanline
		for (int x = x1; x < x2; x++)
		{
			float u = (u1 + (x - x1) * uv_slope_u) * UV_SCALE_FACTOR;
			float v = (v1 + (x - x1) * uv_slope_v) * UV_SCALE_FACTOR;

			// Convert UVs to texture coordinates
			int texU = static_cast<int>(u * texWidth) & (texWidth - 1);
			int texV = static_cast<int>(v * texHeight) & (texHeight - 1);

			// Sample texture color
			uint8_t texColor = texture[texV * texWidth + texU];

			// Apply color if not transparent
			if (texColor != 0xFF)
			{
				g_swapContext.writepage[y * SCREEN_WIDTH + x] = texColor + paletteOffset;
			}
		}
	}
}


