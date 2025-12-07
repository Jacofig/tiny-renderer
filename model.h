#pragma once
#include <vector>
#include <string>

struct Vec3f {
    float x, y, z;
};

class Model {
    public:
        Model(const std::string & filename);

    std::vector<Vec3f> vertices;
    std::vector<std::vector<int>> faces;
};