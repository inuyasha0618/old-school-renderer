#ifndef __MODEL_H__
#define __MODEL_H__
#include <vector>
#include <string>
#include "glm/glm.hpp"
#include "tgaimage.h"

class Model {
private:
    std::vector<glm::vec3> verts_;
    std::vector<std::vector<glm::ivec3> > faces_; // attention, this glm::ivec3 means vertex/uv/normal
    std::vector<glm::vec3> norms_;
    std::vector<glm::vec2> uv_;
    TGAImage diffusemap_;
    TGAImage normalmap_;
    TGAImage specularmap_;
    void load_texture(std::string filename, const char *suffix, TGAImage &img);
public:
    Model(const char *filename);
    ~Model();
    int nverts();
    int nfaces();
    glm::vec3 normal(int iface, int nthvert);
    glm::vec3 normal(glm::vec2 uv);
    glm::vec3 vert(int i);
    glm::vec3 vert(int iface, int nthvert);
    glm::vec2 uv(int iface, int nthvert);
    TGAColor diffuse(glm::vec2 uv);
    float specular(glm::vec2 uv);
    std::vector<int> face(int idx);
};
#endif //__MODEL_H__
