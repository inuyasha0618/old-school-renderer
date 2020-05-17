#include "STC_Gl.h"
#include <iostream>

using namespace std;


BaseShader::~BaseShader() {}

vec3 getBaryCoords(ivec2 *pts, ivec2 P)
{
    ivec2 A = pts[0];
	ivec2 B = pts[1];
	ivec2 C = pts[2];

	ivec2 AB = B - A;
	ivec2 AC = C - A;
	ivec2 PA = A - P;

	vec3 cross_result = cross(vec3(AB.x, AC.x, PA.x), vec3(AB.y, AC.y, PA.y));
	if (std::abs(cross_result.z) < 1.0)
	{
		return vec3(-1.0, 1.0, 1.0);
	}
	cross_result /= cross_result.z;

	return vec3(1.0 - cross_result.x - cross_result.y, cross_result.x, cross_result.y);
}

void rasterization(BaseShader* shader, ivec3 *pts, TGAImage& image, float* zbuffer, int screenWidth, int screenHeight)
{
	// float max_num = numeric_limits<int>::max();
	float max_num = 8000;
	ivec2 bbox_min = ivec2(max_num, max_num);
	ivec2 bbox_max = ivec2(-max_num, -max_num);

	for (int i = 0; i < 3; i++)
	{
		bbox_min.x = std::max(0, std::min(pts[i].x, bbox_min.x));
		bbox_min.y = std::max(0, std::min(pts[i].y, bbox_min.y));
		bbox_max.x = std::max(pts[i].x, bbox_max.x);
		bbox_max.y = std::max(pts[i].y, bbox_max.y);
	}

	ivec3 p;
	vec3 bary_coords;

    ivec2 screenPts[3];

    screenPts[0] = ivec2(pts[0].x, pts[0].y);
    screenPts[1] = ivec2(pts[1].x, pts[1].y);
    screenPts[2] = ivec2(pts[2].x, pts[2].y);

	for (p.x = bbox_min.x; p.x <= bbox_max.x; p.x += 1)
	{
		for (p.y = bbox_min.y; p.y <= bbox_max.y; p.y += 1)
		{
			bary_coords = getBaryCoords(screenPts, ivec2(p.x, p.y));
            float fragDepth = pts[0].z * bary_coords.x + pts[1].z * bary_coords.y + pts[2].z * bary_coords.z;
            p.z = fragDepth;
            fragDepth = std::max(0.f, std::min(fragDepth, DEPTH));

            //                                                                  earlyZ, 先做深度剔除
			if (bary_coords.x < 0 || bary_coords.y < 0 || bary_coords.z < 0 || fragDepth > zbuffer[p.x + p.y * screenWidth]) continue;

            TGAColor OutFragColor;
            shader->PixelShader(OutFragColor, bary_coords, p);
            image.set(p.x, p.y, OutFragColor);
            zbuffer[p.x + p.y * screenWidth] = fragDepth;
		}
	}
}

ivec3 viewport(vec3 NDC, int width, int height)
{
	int x = int((NDC.x + 1.0) * 0.5 * width);
	int y = int((NDC.y + 1.0) * 0.5 * height);
    int z = int((NDC.z + 1.0) * 0.5 * DEPTH);

	return ivec3(x, y, z);
}

void GraphicsPipeLine(Model* model, TGAImage& image, float* zbuffer, BaseShader* shader, mat4& modelMatrix, mat4& viewMatrix, mat4& perspectiveMatrix, int screenWidth, int screenHeight)
{

    for (int i = 0; i < model->nfaces(); i++)
	{
		vector<int> vert_idx_in_currentFace = model->face(i);
		ivec3 screen_pos[3];

		for (int j = 0; j < 3; j++)
		{

			vec3 pos = model->vert(vert_idx_in_currentFace[j]);
            // vec4 P = vec4(pos.x, pos.y, pos.z, 1.0);
            // 调用顶点着色器 得到裁剪空间坐标
            vec4 clipCoord = shader->VertexShader(i, j);

            // 得到Normalize Device Coordinate
            vec3 NDC = vec3(clipCoord.x / clipCoord.w, clipCoord.y / clipCoord.w, clipCoord.z / clipCoord.w);

            // 得到屏幕空间像素坐标(带深度)
			screen_pos[j] = viewport(NDC, screenWidth, screenHeight);

		}

        rasterization(shader, screen_pos, image, zbuffer, screenWidth, screenHeight);
	}
}
