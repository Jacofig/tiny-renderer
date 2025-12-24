#pragma once
#include <vector>
#include <string>
#include "geometry.h"

class Model {
    public:
    Model(const std::string & filename);


    std::vector<vec3> vertices;
    std::vector<std::vector<int>> faces;
    std::vector<int> face_vrt{};
    int nverts() const;
    int nfaces() const;
    vec3 vert(const int i) const;
    vec3 vert(const int iface, const int nthvert) const;
};