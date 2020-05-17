#ifndef __STC_GL_H__
#define __STC_GL_H__

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "tgaimage.h"
#include "model.h"

using namespace glm;
using namespace std;

const float DEPTH = 2000.0f;
struct BaseShader {
    // mat4 uniform_M;   
	// mat4 uniform_V;   
	// mat4 uniform_P;   
    // mat4 uniform_MIT; 
    // mat4 uniform_Mshadow; // transform framebuffer screen coordinates to shadowbuffer screen coordinates
    // mat3x2 varying_uv;  // triangle uv coordinates, written by the vertex shader, read by the fragment shader
	// mat3x4 varying_N;
    virtual ~BaseShader();
    // virtual vec4 VertexShader(vec4& P, mat4& modelMatrix, mat4& viewMatrix, mat4& perspectiveMatrix) = 0;
    virtual vec4 VertexShader(int iFace, int iVert) = 0;
    virtual bool PixelShader(TGAColor& OutFragColor, vec3 baryCoord, ivec3 screenCoords) = 0;
};

void GraphicsPipeLine(Model* model, TGAImage& image, float* zbuffer, BaseShader* shader, mat4& modelMatrix, mat4& viewMatrix, mat4& perspectiveMatrix, int screenWidth, int screenHeight);

#endif
