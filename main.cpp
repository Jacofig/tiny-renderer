#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
using namespace std;
constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};
constexpr int width  = 3000;
constexpr int height = 3000;

double signed_triangle_area(int ax,int ay,int bx,int by,int cx,int cy) {
    return 0.5 * ((by-ay)*(bx+ax) + (cy-by)*(cx+bx) + (ay-cy)*(ax+cx));
}

Vec3f rot(Vec3f v) {
    double a = M_PI / 6.0;

    float x =  v.x * std::cos(a) + v.z * std::sin(a);
    float y =  v.y;
    float z = -v.x * std::sin(a) + v.z * std::cos(a);

    return Vec3f(x, y, z);
}

Vec3f perspective(Vec3f v) {
    double c = 3.0;
    float inv = 1.0f / (1.0f - v.z / c);
    return Vec3f(v.x * inv, v.y * inv, v.z);
}

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

void triangle_bounding_box(int ax, int ay, float az, int bx, int by, float bz, int cx, int cy, float cz, TGAImage &framebuffer, TGAColor color, vector<float>& zbuffer) {
    int boxSmallestX = min(min(ax,bx), cx);
    int boxSmallestY = min(min(ay,by), cy);
    int boxBiggestX = max(max(ax,bx), cx);
    int boxBiggestY = max(max(ay,by), cy);

    double total = signed_triangle_area(ax,ay,bx,by,cx,cy);
    if (total <= 0) return;

    for (int i = boxSmallestX; i <= boxBiggestX; i++) {
        for (int j = boxSmallestY; j <= boxBiggestY; j++) {
            double a = signed_triangle_area(i,j, bx,by, cx,cy) / total;
            double b = signed_triangle_area(i,j, cx,cy, ax,ay) / total;
            double g = 1.0 - a - b;

            if (a < 0 || b < 0 || g < 0) continue;

            float z = float(a*az + b*bz + g*cz);

            int id = i + j * width;
            if (z <= zbuffer[id]) continue;

            zbuffer[id] = z;
            framebuffer.set(i, j, color);
        }
    }
}

int main(int argc, char** argv) {
    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage framebuffer_rasterization(256, 256, TGAImage::RGB);
    vector<float> zbuffer(width*height, -numeric_limits<float>::infinity());

    Model model("diablo3_pose.obj");
    auto mapToScreenX = [&](float x) { return int((x+1.0f) * 0.5f * width); };
    auto mapToScreenY = [&](float y) { return int((y+1.0f) * 0.5 * height); };

    for (auto &face : model.faces) {
        int i0 = face[0];
        int i1 = face[1];
        int i2 = face[2];

        Vec3f v0 = perspective(rot(model.vertices[i0]));
        Vec3f v1 = perspective(rot(model.vertices[i1]));
        Vec3f v2 = perspective(rot(model.vertices[i2]));

        // 1. rotate the scene (camera illusion)
        v0 = rot(v0);
        v1 = rot(v1);
        v2 = rot(v2);

        // 2. perspective projection
        v0 = perspective(v0);
        v1 = perspective(v1);
        v2 = perspective(v2);

        // 3. map to screen
        int x0 = mapToScreenX(v0.x);
        int y0 = mapToScreenY(v0.y);
        int x1 = mapToScreenX(v1.x);
        int y1 = mapToScreenY(v1.y);
        int x2 = mapToScreenX(v2.x);
        int y2 = mapToScreenY(v2.y);

        // 4. depth stays as z AFTER perspective
        float z0 = v0.z;
        float z1 = v1.z;
        float z2 = v2.z;

        TGAColor random;
        for (int c = 0; c < 3; c++) {
            random[c] = rand() % 255;
        }
        //triangle_scanline(x0, y0, x1, y1, x2, y2, framebuffer, random);
        triangle_bounding_box(x0,y0,z0,x1,y1,z1,x2,y2,z2, framebuffer, random, zbuffer);
    }


    framebuffer.write_tga_file("framebuffer.tga");

    triangle_scanline(  7, 45, 35, 100, 45,  60, framebuffer_rasterization, red);
    triangle_scanline(120, 35, 90,   5, 45, 110, framebuffer_rasterization, white);
    triangle_scanline(115, 83, 80,  90, 85, 120, framebuffer_rasterization, green);
    framebuffer_rasterization.write_tga_file("framebuffer2.tga");
    return 0;
}
