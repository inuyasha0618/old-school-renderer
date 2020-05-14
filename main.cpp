#include "tgaimage.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include <math.h>

// const TGAColor white = TGAColor(255, 255, 255, 255);
// const TGAColor red   = TGAColor(255, 0,   0,   255);
using namespace glm;
using namespace std;

Model *model = NULL;

void output_mat(mat4& matrix)
{
	cout << matrix[0][0] << "  " << matrix[0][1] << "  " << matrix[0][2] << "  " << matrix[0][3] << "  " << endl;
	cout << matrix[1][0] << "  " << matrix[1][1] << "  " << matrix[1][2] << "  " << matrix[1][3] << "  " << endl;
	cout << matrix[2][0] << "  " << matrix[2][1] << "  " << matrix[2][2] << "  " << matrix[2][3] << "  " << endl;
	cout << matrix[3][0] << "  " << matrix[3][1] << "  " << matrix[3][2] << "  " << matrix[3][3] << "  " << endl;
}

vec3 getBaryCoords(tvec2<int> *pts, tvec2<int> P)
{
	tvec2<int> A = pts[0];
	tvec2<int> B = pts[1];
	tvec2<int> C = pts[2];

	tvec2<int> AB = B - A;
	tvec2<int> AC = C - A;
	tvec2<int> PA = A - P;

	vec3 cross_result = cross(vec3(AB.x, AC.x, PA.x), vec3(AB.y, AC.y, PA.y));
	if (abs(cross_result.z) < 1.0)
	{
		return vec3(-1.0, 1.0, 1.0);
	}
	cross_result /= cross_result.z;

	return vec3(1.0 - cross_result.x - cross_result.y, cross_result.x, cross_result.y);
}

void rasterization(tvec2<int> *pts, TGAImage& image, TGAColor color)
{
	// float max_num = numeric_limits<int>::max();
	float max_num = 8000;
	tvec2<int> bbox_min = tvec2<int>(max_num, max_num);
	tvec2<int> bbox_max = tvec2<int>(-max_num, -max_num);

	for (int i = 0; i < 3; i++)
	{
		bbox_min.x = std::max(0, std::min(pts[i].x, bbox_min.x));
		bbox_min.y = std::max(0, std::min(pts[i].y, bbox_min.y));
		bbox_max.x = std::max(pts[i].x, bbox_max.x);
		bbox_max.y = std::max(pts[i].y, bbox_max.y);
	}

	// cout << "bbox_min.x: " << bbox_min.x << " bbox_min.y: " << bbox_min.y << " bbox_max.x: " << bbox_max.x << " bbox_max.y: " << bbox_max.y << endl;

	tvec2<int> p;
	vec3 bary_coords;
	for (p.x = bbox_min.x; p.x <= bbox_max.x; p.x += 1)
	{
		for (p.y = bbox_min.y; p.y <= bbox_max.y; p.y += 1)
		{
			bary_coords = getBaryCoords(pts, p);
			if (bary_coords.x < 0 || bary_coords.y < 0 || bary_coords.z < 0) continue;
			image.set(p.x, p.y, color);
		}
	}
}

mat4 getPerspective(float FOV, float aspect, float near, float far)
{
	return mat4(
		1.0 / (aspect * tan(radians(FOV * 0.5))), 0.0, 0.0, 0.0,
		0.0, 1.0 / tan(radians(FOV * 0.5)), 0.0, 0.0,
		0.0, 0.0, (near + far) / (near - far), -1.0,
		0.0, 0.0, 2.0 * near * far / (near - far), 0.0 
	);
}

mat4 getViewMatrix(vec3& eye, vec3& target, vec3& up)
{
	vec3 Z = normalize(eye - target);
	vec3 X = cross(normalize(up), Z);
	vec3 Y = cross(Z, X);

	mat4 View2World = mat4(
		X.x, X.y, X.z, 0.0,
		Y.x, Y.y, Y.z, 0.0,
		Z.x, Z.y, Z.z, 0.0,
		eye.x, eye.y, eye.z, 1.0
	);

	mat3 A = mat3(
		X.x, Y.x, Z.x,
		X.y, Y.y, Z.y,
		X.z, Y.z, Z.z
	);

	vec3 right_col = -A * eye;

	return mat4(
		X.x, Y.x, Z.x, 0.0,
		X.y, Y.y, Z.y, 0.0,
		X.z, Y.z, Z.z, 0.0,
		right_col.x, right_col.y, right_col.z, 1.0
	);
}

tvec2<int> viewport(vec3 NDC, int width, int height)
{
	int x = int((NDC.x + 1.0) * 0.5 * width);
	int y = int((NDC.y + 1.0) * 0.5 * height);
	return tvec2<int>(x, y);
}
// glm构造矩阵式按列传入的， 并且tranlate rotate等是在当前的姿态基础上施加变换
int main(int argc, char** argv) {
	const int ScreenWidth = 800;
	const int ScreenHeight = 800;
	TGAImage image(ScreenWidth, ScreenHeight, TGAImage::RGB);
	// glm::vec2 pts[3] = {vec2(10.0, 10.0), vec2(100.0, 30.0), vec2(190.0, 160.0)};
	// rasterization(pts, image, TGAColor(255, 255, 0, 255));

	if (argc == 2)
	{
		model = new Model(argv[1]);
	}
	else
	{
		model = new Model("./african_head.obj");
	}
	
	vec3 eye = vec3(0.0, 0.0, 2.0);
	vec3 target = vec3(0.0, 0.0, 0.0);
	vec3 up = vec3(0.0, 1.0, 0.0);
	mat4 ViewMatrix = getViewMatrix(eye, target, up);
	mat4 PerspectiveMatrix = getPerspective(90, 1.0, 0.1, 20);

	for (int i = 0; i < model->nfaces(); i++)
	{
		vector<int> vert_idx_in_currentFace = model->face(i);
		tvec2<int> screen_pos[3];
		for (int j = 0; j < 3; j++)
		{
			// screen_pos[j].x = int((model->vert(vert_idx_in_currentFace[j]).x + 1) * 0.5 * ScreenWidth);
			// screen_pos[j].y = int((model->vert(vert_idx_in_currentFace[j]).y + 1) * 0.5 * ScreenHeight);
			vec3 pos = model->vert(vert_idx_in_currentFace[j]);
			vec4 worldPos = vec4(pos.x, pos.y, pos.z, 1.0);
			vec4 viewPos = ViewMatrix * worldPos;
			vec4 clipPos = PerspectiveMatrix * viewPos;
			vec3 NDC = vec3(clipPos.x / clipPos.w, clipPos.y / clipPos.w, clipPos.z / clipPos.w);

			screen_pos[j] = viewport(NDC, ScreenWidth, ScreenHeight);
			// screen_pos[j] = tvec2<int>((model->vert(vert_idx_in_currentFace[j]).x + 1) * 0.5 * ScreenWidth, (model->vert(vert_idx_in_currentFace[j]).y + 1) * 0.5 * ScreenHeight);
		}

		// rasterization(screen_pos, image, TGAColor(rand() % 255, rand() % 255, rand() % 255, 255));
		vec3 pt0 = model->vert(vert_idx_in_currentFace[0]);
		vec3 pt1 = model->vert(vert_idx_in_currentFace[1]);
		vec3 pt2 = model->vert(vert_idx_in_currentFace[2]);
		vec3 flat_normal = normalize(cross(pt2 - pt0, pt1 - pt0));
		vec3 light_dir = vec3(0.0, 0.0, -1.0);
		float intensity = dot(flat_normal, light_dir);
		if (intensity > 0)
			rasterization(screen_pos, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
	}
	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("framebuffer.tga");
	// system("pause");
	return 0;
}