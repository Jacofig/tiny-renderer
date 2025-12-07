#include <cmath>
#include "tgaimage.h"
#include "model.h"
using namespace std;
constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};


void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool isTall = abs(ay-by) > abs(ax-bx);
    if (isTall) {
        swap(ax, ay);
        swap(bx, by);
    }
    if (ax > bx) {
        swap(ax, bx);
        swap(ay, by);
    }
    float y = ay;
    int error = 0;
    float slope = (by-ay) / float(bx-ax);
    for (float x=ax; x <= bx; x++) {
        if (isTall)
            framebuffer.set(y, x, color);
        else
            framebuffer.set(x, y, color);

        error += 2*abs(by-ay);
        if (error > bx-ax) {
            y += by > ay ? 1 : -1;
            error -= 2 * (bx-ax);
        }
    }
}

int main(int argc, char** argv) {
    constexpr int width  = 3000;
    constexpr int height = 3000;
    TGAImage framebuffer(width, height, TGAImage::RGB);
    Model model("diablo3_pose.obj");
    auto mapToScreenX = [&](float x) { return int((x+1.0f) * 0.5f * width); };
    auto mapToScreenY = [&](float y) { return int((y+1.0f) * 0.5 * height); };

    for (auto &face : model.faces) {
        int i0 = face[0];
        int i1 = face[1];
        int i2 = face[2];

        Vec3f v0 = model.vertices[i0];
        Vec3f v1 = model.vertices[i1];
        Vec3f v2 = model.vertices[i2];

        int x0 = mapToScreenX(v0.x);
        int y0 = mapToScreenY(v0.y);
        int x1 = mapToScreenX(v1.x);
        int y1 = mapToScreenY(v1.y);
        int x2 = mapToScreenX(v2.x);
        int y2 = mapToScreenY(v2.y);

        line(x0, y0, x1, y1, framebuffer, white);
        line(x1, y1, x2, y2, framebuffer, white);
        line(x2, y2, x0, y0, framebuffer, white);
    }


    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
