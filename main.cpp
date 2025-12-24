#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "gl.h"
#include "config.h"
#include <iostream>
using namespace std;
struct RandomShader : IShader {
    const Model &model;
    vec3 tri[3];

    RandomShader(const Model &model) : model(model) {}

    virtual vec4 vertex(const int face, const int vert) {
        vec3 v = model.vert(face, vert);
        vec4 view_pos = ModelView * vec4{v.x, v.y, v.z, 1.0};

        tri[vert] = view_pos.xyz();  // view space
        return Perspective * view_pos;
    }

    virtual pair<bool, TGAColor> fragment(const vec3 bar) const {
        vec3 n = normalized(cross(tri[1] - tri[0], tri[2] - tri[0]));
        TGAColor color;
        color[0] = (n.x * 0.5 + 0.5) * 255;
        color[1] = (n.y * 0.5 + 0.5) * 255;
        color[2] = (n.z * 0.5 + 0.5) * 255;
        color[3] = 255;
        return {false, color};
    }
};
int main() {
    TGAImage framebuffer(width, height, TGAImage::RGB);
    vector<float> zbuffer(width * height, -numeric_limits<float>::infinity());

    constexpr vec3 eye{-1, 0, 2};
    constexpr vec3 center{0, 0, 0};
    constexpr vec3 up{0, 1, 0};

    lookat(eye, center, up);
    perspective(norm(eye - center));
    viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);

    Model model("diablo3_pose.obj");
    RandomShader shader(model);

    for (int face = 0; face < model.nfaces(); face++) {
        Triangle clip;
        clip[0] = shader.vertex(face, 0);
        clip[1] = shader.vertex(face, 1);
        clip[2] = shader.vertex(face, 2);

        rasterize(clip, shader, framebuffer, zbuffer);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
