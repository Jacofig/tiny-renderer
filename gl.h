#include "tgaimage.h"
#include "geometry.h"

using namespace std;

extern mat<4,4> ModelView;
extern mat<4,4> Viewport;
extern mat<4,4> Perspective;

void lookat(const vec3 eye, const vec3 center, const vec3 up);
void perspective(const double f);
void viewport(const int x, const int y, const int w, const int h);

struct IShader {
    virtual pair<bool, TGAColor> fragment(const vec3 bar) const = 0;
};

typedef vec4 Triangle[3];
double signed_triangle_area(int ax,int ay,int bx,int by,int cx,int cy);
void rasterize(const Triangle &clip, const IShader &shader, TGAImage &framebuffer, std::vector<float> &zbuffer);
void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color);
void triangle_scanline(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color);
