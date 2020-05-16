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

void rasterization(BaseShader* shader, ivec3 *pts, vec3* Ns, TGAImage& image, TGAColor color, TGAImage& zbuffer, vec3 lightDir)
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

	// cout << "bbox_min.x: " << bbox_min.x << " bbox_min.y: " << bbox_min.y << " bbox_max.x: " << bbox_max.x << " bbox_max.y: " << bbox_max.y << endl;

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
            int fragDepth = pts[0].z * bary_coords.x + pts[1].z * bary_coords.y + pts[2].z * bary_coords.z;
            fragDepth = std::max(0, std::min(fragDepth, 255));

            //                                                                  earlyZ, 先做深度剔除
			if (bary_coords.x < 0 || bary_coords.y < 0 || bary_coords.z < 0 || fragDepth > zbuffer.get(p.x, p.y)[0]) continue;
			// if (bary_coords.x < 0 || bary_coords.y < 0 || bary_coords.z < 0) continue;
			vec3 N = mat3(Ns[0], Ns[1], Ns[2]) * bary_coords;
            N = normalize(N);
            vec3 OutFragColor = vec3();
            shader->PixelShader(OutFragColor, vec2(), N, lightDir);
            image.set(p.x, p.y, TGAColor(OutFragColor.r * 255, OutFragColor.g * 255, OutFragColor.b * 255));
            zbuffer.set(p.x, p.y, TGAColor(fragDepth, fragDepth, fragDepth, fragDepth));
		}
	}
}

ivec3 viewport(vec3 NDC, int width, int height)
{
	int x = int((NDC.x + 1.0) * 0.5 * width);
	int y = int((NDC.y + 1.0) * 0.5 * height);
    int z = int((NDC.z + 1.0) * 0.5 * 255);

	return ivec3(x, y, z);
}

void GraphicsPipeLine(Model* model, TGAImage& image, BaseShader* shader, mat4& modelMatrix, mat4& viewMatrix, mat4& perspectiveMatrix, int screenWidth, int screenHeight, vec3 LightDir)
{
    TGAImage zbuffer(screenWidth, screenHeight, TGAImage::RGBA);
    for (int x = 0; x < screenWidth; x++)
    {
        for (int y = 0; y < screenHeight; y++)
        {
            zbuffer.set(x, y, TGAColor(255, 255, 255, 255));
        }
    }

    for (int i = 0; i < model->nfaces(); i++)
	{
		vector<int> vert_idx_in_currentFace = model->face(i);
		ivec3 screen_pos[3];
        vec2 uv[3];
        vec3 N[3];
		for (int j = 0; j < 3; j++)
		{

			vec3 pos = model->vert(vert_idx_in_currentFace[j]);
            vec4 P = vec4(pos.x, pos.y, pos.z, 1.0);
            // 调用顶点着色器 得到裁剪空间坐标
            vec4 clipCoord = shader->VertexShader(P, modelMatrix, viewMatrix, perspectiveMatrix);

            // 得到Normalize Device Coordinate
            vec3 NDC = vec3(clipCoord.x / clipCoord.w, clipCoord.y / clipCoord.w, clipCoord.z / clipCoord.w);

            // 得到屏幕空间像素坐标(带深度)
			screen_pos[j] = viewport(NDC, screenWidth, screenHeight);
            uv[j] = model->uv(i, j);
            N[j] = model->normal(i, j);
		}

		vec3 pt0 = model->vert(vert_idx_in_currentFace[0]);
		vec3 pt1 = model->vert(vert_idx_in_currentFace[1]);
		vec3 pt2 = model->vert(vert_idx_in_currentFace[2]);
		vec3 flat_normal = normalize(cross(pt2 - pt0, pt1 - pt0));
		vec3 light_dir = vec3(0.0, 0.0, -1.0);
		float intensity = dot(flat_normal, light_dir);
		if (intensity > 0)
			rasterization(shader, screen_pos, N, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255), zbuffer, LightDir);
	}
}
