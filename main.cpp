#include "tgaimage.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include <math.h>
#include "STC_Gl.h"
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

struct PhongShader: BaseShader
{
	virtual vec4 VertexShader(vec4& P, mat4& modelMatrix, mat4& viewMatrix, mat4& perspectiveMatrix) {
		return vec4();
	}
    virtual bool PixelShader() {
		return false;
	}
};


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
	
	PhongShader shader;

	vec3 eye = vec3(0.0, 0.0, 2.0);
	vec3 target = vec3(0.0, 0.0, 0.0);
	vec3 up = vec3(0.0, 1.0, 0.0);

	mat4 ModelMatrix = mat4();
	mat4 ViewMatrix = getViewMatrix(eye, target, up);
	mat4 PerspectiveMatrix = getPerspective(90, 1.0, 0.1, 20);
	
	GraphicsPipeLine(model, image, &shader, ModelMatrix, ViewMatrix, PerspectiveMatrix, ScreenWidth, ScreenHeight);

	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("framebuffer.tga");
	// system("pause");
	return 0;
}