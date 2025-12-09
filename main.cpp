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

void triangle_scanline(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    if (ay>by) { swap(ax, bx); swap(ay, by); }
    if (ay>cy) { swap(ax, cx); swap(ay, cy); }
    if (by>cy) { swap(bx, cx); swap(by, cy); }
    int height = cy - ay;
    if (ay != by) {
        int bottom_half_height = by - ay;
        for (int y =ay; y <= by; y++) {
            int x1 = ax + ((cx - ax) * (y-ay)) / height;
            int x2 = ax + ((bx - ax) * (y-ay)) / bottom_half_height;
            for (int x = min(x1,x2); x < max(x1,x2); x++) {
                framebuffer.set(x, y, color);
            }
        }
    }
    if (by != cy) {
        int upper_half_height = cy - by;
        for (int y=by; y<=cy; y++) {
            int x1 = ax + ((cx - ax)*(y - ay)) / height;
            int x2 = bx + ((cx - bx)*(y - by)) / upper_half_height;
            for (int x=min(x1,x2); x<max(x1,x2); x++)
                framebuffer.set(x, y, color);
        }
    }
}

bool is_in_triangle(int ax, int ay, int bx, int by, int cx, int cy, int px, int py) {
    double w1 = 0.0, w2 = 0.0;

    // calculate w1
    double denom1 = static_cast<double>((by - ay) * (cx - ax) - (bx - ax) * (cy - ay));
    if (denom1 == 0.0) return false; // degenerate triangle
    double num1 = static_cast<double>(ax * (cy - ay) + (py - ay) * (cx - ax) - px * (cy - ay));
    w1 = num1 / denom1;

    // calculate w2
    double denom2 = static_cast<double>(cy - ay);
    if (denom2 == 0.0) return false;
    w2 = (static_cast<double>(py - ay) - w1 * (by - ay)) / denom2;

    if (w1 >= 0.0 && w2 >= 0.0 && w1 + w2 <= 1.0)
        return true;
    else
        return false;
}

void triangle_bounding_box(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    int boxSmallestX = min(min(ax,bx), cx);
    int boxSmallestY = min(min(ay,by), cy);
    int boxBiggestX = max(max(ax,bx), cx);
    int boxBiggestY = max(max(ay,by), cy);

    for (int i = boxSmallestX; i <= boxBiggestX; i++) {
        for (int j = boxSmallestY; j <= boxBiggestY; j++) {
            if (is_in_triangle(ax,ay,bx,by,cx,cy,i,j))
                framebuffer.set(i, j, color);
        }
    }
}

int main(int argc, char** argv) {
    constexpr int width  = 3000;
    constexpr int height = 3000;
    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage framebuffer_rasterization(256, 256, TGAImage::RGB);

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
        TGAColor random;
        for (int c = 0; c < 3; c++) {
            random[c] = rand() % 255;
        }
        //triangle_scanline(x0, y0, x1, y1, x2, y2, framebuffer, random);
        triangle_bounding_box(x0,y0,x1,y1,x2,y2, framebuffer, random);
    }


    framebuffer.write_tga_file("framebuffer.tga");

    triangle_scanline(  7, 45, 35, 100, 45,  60, framebuffer_rasterization, red);
    triangle_scanline(120, 35, 90,   5, 45, 110, framebuffer_rasterization, white);
    triangle_scanline(115, 83, 80,  90, 85, 120, framebuffer_rasterization, green);
    framebuffer_rasterization.write_tga_file("framebuffer2.tga");
    return 0;
}
