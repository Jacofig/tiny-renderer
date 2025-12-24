#include "model.h"
#include <sstream>
#include <fstream>
#include <iostream>

Model::Model(const std::string &filename) {
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "Error opening file " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            float x, y, z;
            ss >> x >> y >> z;
            vertices.push_back(vec3(x, y, z));
        }
        else if (type == "f") {
            std::string a, b, c;
            ss >> a >> b >> c;

            auto getIndex = [](const std::string &s) {
                return std::stoi(s.substr(0, s.find('/'))) - 1;
            };

            int i0 = getIndex(a);
            int i1 = getIndex(b);
            int i2 = getIndex(c);

            faces.push_back({i0, i1, i2});
        }
    }
}

int Model::nverts() const { return vertices.size(); }
int Model::nfaces() const { return faces.size(); }

vec3 Model::vert(const int i) const {
    return vertices[i];
}

vec3 Model::vert(const int iface, const int nthvert) const {
    return vertices[faces[iface][nthvert]];
}
