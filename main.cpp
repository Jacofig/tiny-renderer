#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
using namespace std;
constexpr int width  = 3000;
constexpr int height = 3000;
mat<4,4> ModelView, Viewport, Perspective;

// compute signed area
// positive value -> points are counter-clockwise
// negative value -> points are clockwise
// 0 -> degenerate (is a line)
double signed_triangle_area(int ax,int ay,int bx,int by,int cx,int cy) {
    return 0.5 * ((by-ay)*(bx+ax) + (cy-by)*(cx+bx) + (ay-cy)*(ax+cx));
}

// rotate around Y axis
// using rotation matrix
// [cos(a)  0   sin(a)]
// [0       1      0  ]
// [-sin(a) 0   cos(a)]

Vec3f rot(Vec3f v) {
    double a = M_PI / 6.0;

    float x =  v.x * cos(a) + v.z * sin(a);
    float y =  v.y; // y stays the same
    float z = -v.x * sin(a) + v.z * cos(a);

    return Vec3f(x, y, z);
}

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

    boxSmallestX = std::max(0, boxSmallestX);
    boxSmallestY = std::max(0, boxSmallestY);
    boxBiggestX  = std::min(width  - 1, boxBiggestX);
    boxBiggestY  = std::min(height - 1, boxBiggestY);

    // if completely outside
    if (boxSmallestX > boxBiggestX || boxSmallestY > boxBiggestY) return;

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
    constexpr vec3    eye{-1,0,2}; // camera position
    constexpr vec3 center{0,0,0};  // camera direction
    constexpr vec3     up{0,1,0};  // camera up vector

    lookat(eye, center, up);                              // build the ModelView   matrix
    perspective(norm(eye-center));                        // build the Perspective matrix
    viewport(width/16, height/16, width*7/8, height*7/8); // build the Viewport    matrix

    Model model("diablo3_pose.obj");
    auto mapToScreenX = [&](float x) { return int((x+1.0f) * 0.5f * width); };
    auto mapToScreenY = [&](float y) { return int((y+1.0f) * 0.5 * height); };

    for (auto &face : model.faces) {
        int i0 = face[0], i1 = face[1], i2 = face[2];

        vec4 c0 = Perspective * ModelView * vec4{
            model.vertices[i0].x,
            model.vertices[i0].y,
            model.vertices[i0].z,
            1.0
        };
        vec4 c1 = Perspective * ModelView * vec4{
            model.vertices[i1].x,
            model.vertices[i1].y,
            model.vertices[i1].z,
            1.0
        };
        vec4 c2 = Perspective * ModelView * vec4{
            model.vertices[i2].x,
            model.vertices[i2].y,
            model.vertices[i2].z,
            1.0
        };

        // avoid division by zero / behind-camera issues
        if (fabs(c0.w) < 1e-6 || fabs(c1.w) < 1e-6 || fabs(c2.w) < 1e-6)
            continue;

        vec4 n0 = c0 / c0.w;
        vec4 n1 = c1 / c1.w;
        vec4 n2 = c2 / c2.w;

        vec4 s0h = Viewport * n0;
        vec4 s1h = Viewport * n1;
        vec4 s2h = Viewport * n2;

        vec3 s0 = s0h.xyz();
        vec3 s1 = s1h.xyz();
        vec3 s2 = s2h.xyz();

        int x0 = int(s0.x), y0 = int(s0.y);
        int x1 = int(s1.x), y1 = int(s1.y);
        int x2 = int(s2.x), y2 = int(s2.y);

        float z0 = s0.z;
        float z1 = s1.z;
        float z2 = s2.z;

        TGAColor random;
        for (int c = 0; c < 3; c++) random[c] = rand() % 255;

        triangle_bounding_box(
            x0, y0, z0,
            x1, y1, z1,
            x2, y2, z2,
            framebuffer, random, zbuffer
        );
    }


    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
