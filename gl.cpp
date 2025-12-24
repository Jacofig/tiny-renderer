#include <algorithm>
#include "gl.h"
#include "config.h"
#include <iostream>

mat<4,4> ModelView, Viewport, Perspective;
void lookat(const vec3 eye, const vec3 center, const vec3 up) {
    vec3 z = normalized(eye - center);      // camera forward
    vec3 x = normalized(cross(up, z));      // camera right
    vec3 y = normalized(cross(z, x));       // camera up

    mat<4,4> Minv = {{{x.x,x.y,x.z,0},
                      {y.x,y.y,y.z,0},
                      {z.x,z.y,z.z,0},
                      {0,  0,  0,  1}}};

    mat<4,4> Tr   = {{{1,0,0,-eye.x},
                      {0,1,0,-eye.y},
                      {0,0,1,-eye.z},
                      {0,0,0,1}}};

    ModelView = Minv * Tr;
}

void perspective(const double f) {
    Perspective = {{{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0, -1/f,1}}};
}

void viewport(const int x, const int y, const int w, const int h) {
    const int depth = 255; // or 255, or 1024; choose your zbuffer convention
    Viewport = {{{w/2., 0,    0, x+w/2.},
                 {0,    h/2., 0, y+h/2.},
                 {0,    0, depth/2., depth/2.},
                 {0,    0,    0, 1}}};
}

double signed_triangle_area(int ax,int ay,int bx,int by,int cx,int cy) {
    return 0.5 * ((by-ay)*(bx+ax) + (cy-by)*(cx+bx) + (ay-cy)*(ax+cx));
}
void rasterize(const Triangle &clip, const IShader &shader, TGAImage &framebuffer, std::vector<float> &zbuffer) {
    static bool once = false;
    // --- clip space → screen space ---
    vec3 pts[3];
    for (int i = 0; i < 3; i++) {
        vec4 ndc = clip[i] / clip[i].w;   // perspective divide
        vec4 scr = Viewport * ndc;        // viewport transform
        pts[i] = scr.xyz();
    }

    // --- bounding box ---
    int minX = width - 1, minY = height - 1;
    int maxX = 0, maxY = 0;

    for (int i = 0; i < 3; i++) {
        minX = std::max(0, std::min(minX, int(pts[i].x)));
        minY = std::max(0, std::min(minY, int(pts[i].y)));
        maxX = std::min(width  - 1, std::max(maxX, int(pts[i].x)));
        maxY = std::min(height - 1, std::max(maxY, int(pts[i].y)));
    }

    double total = signed_triangle_area(
        pts[0].x, pts[0].y,
        pts[1].x, pts[1].y,
        pts[2].x, pts[2].y
    );
    if (total <= 0) return;

    // --- rasterization ---
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {

            // barycentric coordinates (inline, same as before)
            double a = signed_triangle_area(
                x, y,
                pts[1].x, pts[1].y,
                pts[2].x, pts[2].y
            ) / total;

            double b = signed_triangle_area(
                x, y,
                pts[2].x, pts[2].y,
                pts[0].x, pts[0].y
            ) / total;

            double g = 1.0 - a - b;

            if (a < 0 || b < 0 || g < 0) continue;

            // depth interpolation
            float z = float(
                pts[0].z * a +
                pts[1].z * b +
                pts[2].z * g
            );

            int id = x + y * width;
            if (z <= zbuffer[id]) continue;

            // fragment shader
            auto [discard, color] = shader.fragment(vec3{a, b, g});
            if (discard) continue;

            zbuffer[id] = z;
            framebuffer.set(x, y, color);
        }
    }
}

// draw a line
void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    // steep lines swap pixels, so iterate over the dominant axis
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

// fill a triangle by horizontal lines
void triangle_scanline(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    // sort vertices by height
    if (ay>by) { swap(ax, bx); swap(ay, by); }
    if (ay>cy) { swap(ax, cx); swap(ay, cy); }
    if (by>cy) { swap(bx, cx); swap(by, cy); }
    int height = cy - ay;
    if (ay != by) {
        int upper_half_height = by - ay;
        for (int y =ay; y <= by; y++) {
            int x1 = ax + ((cx - ax) * (y-ay)) / height;
            int x2 = ax + ((bx - ax) * (y-ay)) / upper_half_height;
            for (int x = min(x1,x2); x < max(x1,x2); x++) {
                framebuffer.set(x, y, color);
            }
        }
    }
    if (by != cy) {
        int lower_half_height = cy - by;
        for (int y=by; y<=cy; y++) {
            int x1 = ax + ((cx - ax)*(y - ay)) / height;
            int x2 = bx + ((cx - bx)*(y - by)) / lower_half_height;
            for (int x=min(x1,x2); x<max(x1,x2); x++)
                framebuffer.set(x, y, color);
        }
    }
}