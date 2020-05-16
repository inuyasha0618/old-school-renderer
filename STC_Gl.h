#ifndef __STC_GL_H__
#define __STC_GL_H__

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "tgaimage.h"
#include "model.h"

using namespace glm;
using namespace std;

struct BaseShader {
    virtual ~BaseShader();
    virtual vec4 VertexShader(vec4& P, mat4& modelMatrix, mat4& viewMatrix, mat4& perspectiveMatrix) = 0;
    virtual bool PixelShader(vec3& OutFragColor, vec2 uv, vec3 N, vec3 LightDir) = 0;
};

void GraphicsPipeLine(Model* model, TGAImage& image, BaseShader* shader, mat4& modelMatrix, mat4& viewMatrix, mat4& perspectiveMatrix, int screenWidth, int screenHeight, vec3 LightDir);

#endif
