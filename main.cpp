#include "tgaimage.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include <math.h>
#include "STC_Gl.h"

// using namespace glm;
// using namespace std;
const int ScreenWidth = 800;
const int ScreenHeight = 800;
float* zbuffer = new float[ScreenWidth * ScreenWidth];
float* zbuffer_for_shadowpass = new float[ScreenWidth * ScreenWidth];

Model *model = NULL;
const vec3 LightDir = normalize(vec3(2.0, 0.0, 0.5));
void output_mat(mat4& matrix)
{
	cout << matrix[0][0] << "  " << matrix[0][1] << "  " << matrix[0][2] << "  " << matrix[0][3] << "  " << endl;
	cout << matrix[1][0] << "  " << matrix[1][1] << "  " << matrix[1][2] << "  " << matrix[1][3] << "  " << endl;
	cout << matrix[2][0] << "  " << matrix[2][1] << "  " << matrix[2][2] << "  " << matrix[2][3] << "  " << endl;
	cout << matrix[3][0] << "  " << matrix[3][1] << "  " << matrix[3][2] << "  " << matrix[3][3] << "  " << endl;
}

mat4 getOtho(float size, float aspect, float near, float far)
{
	return mat4(
		1.0 / (aspect * size), 0.0, 0.0, 0.0,
		0.0, 1.0 / size, 0.0, 0.0,
		0.0, 0.0, 2.0 / (near - far), 0.0,
		0.0, 0.0, (near + far) / (near - far), 1.0 
	);
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
	mat4 uniform_M;   
	mat4 uniform_V;   
	mat4 uniform_P;   
    mat4 uniform_MIT; 
    mat4 uniform_shadowMVP; // transform framebuffer screen coordinates to shadowbuffer screen coordinates
    mat3x2 varying_uv;  // triangle uv coordinates, written by the vertex shader, read by the fragment shader
	mat3x3 varying_N;
	mat3x3 varying_modelP;
	mat3x3 varying_worldP;

	PhongShader() {}
    PhongShader(mat4 M, mat4 V, mat4 P, mat4 MIT, mat4 MS) : uniform_M(M), uniform_V(V), uniform_P(P), uniform_MIT(MIT), uniform_shadowMVP(MS), varying_uv(), varying_N() {}
	
	virtual vec4 VertexShader(int iFace, int iVert) {
		vec3 model_p = model->vert(iFace, iVert);
		vec4 P(model_p.x, model_p.y, model_p.z, 1.0);
		vec3 normal = model->normal(iFace, iVert);
		vec4 N = vec4(normal.x, normal.y, normal.z, 0.0);
		varying_uv[iVert] = model->uv(iFace, iVert);
		varying_N[iVert] = uniform_MIT * N;
		varying_modelP[iVert] = model_p;
		varying_worldP[iVert] = uniform_M * P;
		return uniform_P * uniform_V * uniform_M * P;
	}
    virtual bool PixelShader(TGAColor& OutFragColor, vec3 baryCoord, ivec3 screenCoords) {
		vec2 uv = varying_uv * baryCoord; 
		vec3 N = normalize(varying_N * baryCoord);
		vec3 modelP = varying_modelP * baryCoord;
		// OutFragColor = model->diffuse(uv) * std::max(float(0.0), dot(N, LightDir));
		vec4 clip_shadow = uniform_shadowMVP * vec4(modelP.x, modelP.y, modelP.z, 1.0);
		int shadow_screenCoord_x = ((clip_shadow.x / clip_shadow.w) + 1) * 0.5 * ScreenWidth;
		int shadow_screenCoord_y = ((clip_shadow.y / clip_shadow.w) + 1) * 0.5 * ScreenHeight;
		float depth_in_shadow = ((clip_shadow.z / clip_shadow.w) + 1) * 0.5 * DEPTH;
		float depth_in_shadowbuffer = zbuffer_for_shadowpass[shadow_screenCoord_x + shadow_screenCoord_y * ScreenWidth];
		float shadow = 0.3 + 0.7 * (depth_in_shadow < depth_in_shadowbuffer + 30.0f);

		mat3 A;
		A[0] = varying_modelP[1] - varying_modelP[0];
		A[1] = varying_modelP[2] - varying_modelP[0];
		A[2] = N;
		A = transpose(A);
		mat3 A_inv = inverse(A);
		
		vec3 b1 = vec3(varying_uv[1][0] - varying_uv[0][0], varying_uv[2][0] - varying_uv[0][0], 0.0);
		vec3 b2 = vec3(varying_uv[1][1] - varying_uv[0][1], varying_uv[2][1] - varying_uv[0][1], 0.0);

		mat3 TBN;
		TBN[0] = A_inv * b1;
		TBN[1] = A_inv * b2;
		TBN[2] = N;

		vec3 world_space_normal = normalize(TBN * model->normal(uv));

		vec3 r = normalize((N * (dot(N, LightDir)*2.f) - LightDir));   // reflected light
        float spec = pow(std::max(r.z, 0.0f), model->specular(uv));

		// cout << "depth_in_shadow: " << depth_in_shadow << " depth_in_shadowbuffer: " << depth_in_shadowbuffer << endl;
		// OutFragColor = model->diffuse(uv) * (std::max(float(0.0), dot(world_space_normal, LightDir)) + spec * 0.6) * shadow;
		OutFragColor = model->diffuse(uv) * (std::max(float(0.0), dot(world_space_normal, LightDir))) * shadow;
		OutFragColor[0] += 20;
		OutFragColor[1] += 20;
		OutFragColor[2] += 20;
		// OutFragColor = model->diffuse(uv) * std::max(float(0.0), dot(N, LightDir)) * shadow;

		// OutFragColor = model->diffuse(uv) * pow(std::max(r.z, 0.0f), model->specular(uv));
		// OutFragColor = model->diffuse(uv) * model->specular(uv);
		return false;
	}
};

struct ShadowShader: BaseShader
{
	mat4 uniform_M;   
	mat4 uniform_V;   
	mat4 uniform_P;   

	ShadowShader() {}
    ShadowShader(mat4 M, mat4 V, mat4 P) : uniform_M(M), uniform_V(V), uniform_P(P){}
	
	virtual vec4 VertexShader(int iFace, int iVert) {
		vec3 model_p = model->vert(iFace, iVert);
		vec4 P(model_p.x, model_p.y, model_p.z, 1.0);
		return uniform_P * uniform_V * uniform_M * P;
	}
    virtual bool PixelShader(TGAColor& OutFragColor, vec3 baryCoord, ivec3 screenCoords) {

		OutFragColor = TGAColor(255, 255, 255) * (screenCoords.z / DEPTH);
		return false;
	}
};


// glm构造矩阵式按列传入的， 并且tranlate rotate等是在当前的姿态基础上施加变换
int main(int argc, char** argv) {

	if (argc == 2)
	{
		model = new Model(argv[1]);
	}
	else
	{
		model = new Model("./african_head.obj");
	}
	
	TGAImage image(ScreenWidth, ScreenHeight, TGAImage::RGB);
	TGAImage shadowMap(ScreenWidth, ScreenHeight, TGAImage::RGB);

	for (int i = 0; i < ScreenWidth * ScreenHeight; i++)
	{
		zbuffer_for_shadowpass[i] = zbuffer[i] = std::numeric_limits<float>::max();
	}

	mat4 shadow_mvp;
	// shadow path
	{
		vec3 eye = LightDir * 2.0f;
		vec3 target = vec3(0.0, 0.0, 0.0);
		vec3 up = vec3(0.0, 1.0, 0.0);

		mat4 ModelMatrix = mat4();
		mat4 ViewMatrix = getViewMatrix(eye, target, up);
		mat4 PerspectiveMatrix = getOtho(1.0f, ScreenWidth / ScreenHeight, 0.1, 10);

		ShadowShader shadowShader(ModelMatrix, ViewMatrix, PerspectiveMatrix);

		GraphicsPipeLine(model, shadowMap, zbuffer_for_shadowpass, &shadowShader, ModelMatrix, ViewMatrix, PerspectiveMatrix, ScreenWidth, ScreenHeight);

		shadowMap.flip_vertically(); // i want to have the origin at the left bottom corner of the image
		shadowMap.write_tga_file("shadowMap.tga");
		shadow_mvp = PerspectiveMatrix * ViewMatrix * ModelMatrix;
	}

	// light pass
	{
		vec3 eye = vec3(0.0, 0.0, 1.5);
		vec3 target = vec3(0.0, 0.0, 0.0);
		vec3 up = vec3(0.0, 1.0, 0.0);

		mat4 ModelMatrix = mat4();
		mat4 ViewMatrix = getViewMatrix(eye, target, up);
		mat4 PerspectiveMatrix = getPerspective(90, 1.0, 0.1, 20);

		mat4 MVP = PerspectiveMatrix * ViewMatrix * ModelMatrix;
		mat4 shadow_M = shadow_mvp * inverse(MVP);
		vec3 lightDir = normalize(vec3(1.0, 0.0, 1.0));

		PhongShader shader(ModelMatrix, ViewMatrix, PerspectiveMatrix, inverse(transpose(ModelMatrix)), shadow_mvp);

		GraphicsPipeLine(model, image, zbuffer, &shader, ModelMatrix, ViewMatrix, PerspectiveMatrix, ScreenWidth, ScreenHeight);

		image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
		image.write_tga_file("framebuffer.tga");
	}

	delete model;
    delete [] zbuffer;
	return 0;
}