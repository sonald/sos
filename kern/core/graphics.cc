#include "graphics.h"
#include "common.h"
#include "vm.h"
#include "string.h"
#include "font.h"
#include "spinlock.h"

Spinlock videolock("video");

VideoMode videoMode;
Rgb colormap[] = {
    0x000000,
    0x0000ff,
    0x00ff00,
    0x00ffff,
    0xff0000,
    0xff00ff,
    0xa52a2a,
    0xd3d3d3,
    0xbebebe,
    0xadd8e6,
    0x90ee90,
    0xe0ffff,
    0xcd5c5c,
    0xee00ee,
    0xff4040,
    0xffffff,
    0xffff00,
    0xfafad2,
    0x800080
};

void VideoMode::init(ModeInfoBlock_t* modeinfo)
{
    _base = (char*)0xE0000000;
    uint32_t video_phys = modeinfo->physbase;
    uint32_t vramsize = modeinfo->Yres * modeinfo->pitch;
    vmm.map_pages(vmm.kernel_page_directory(), (void*)_base,
            vramsize, video_phys, PDE_WRITABLE);
    _pitch = modeinfo->pitch;
    _width = modeinfo->Xres;
    _height = modeinfo->Yres;
}

void VideoMode::drawLine(int x0, int y0, int x1, int y1, Rgb rgb)
{
    drawLine({x0, y0}, {x1, y1}, rgb);
}

void VideoMode::octant0(position_t p1, position_t p2, Rgb rgb)
{
    int dx = p2.x - p1.x;
    int dy = p2.y - p1.y;
    int dir = dy >= 0 ? 1 : -1;
    dy = dir * dy;
    int dx2 = dx * 2, dy2 = dy * 2;
    int f = dy2 - dir * dx, y = p1.y;

    for (int x = p1.x; x <= p2.x; x++) {
        if (f >= 0) {
            y += dir;
            drawPixel(x, y, rgb);
            f += dy2 - dx2;
        } else {
            drawPixel(x, y, rgb);
            f += dy2;
        }
    }
}

void VideoMode::octant1(position_t p1, position_t p2, Rgb rgb)
{
    int dx = p2.x - p1.x;
    int dy = p2.y - p1.y;
    int dx2 = dx * 2, dy2 = dy * 2;
    int dir = dy >= 0 ? 1 : -1;
    dy = dir * dy;
    int f = dx2 - dy, x = p1.x;

    for (int y = p1.y; y != p2.y; y += dir) {
        if (f >= 0) {
            drawPixel(++x, y, rgb);
            f += dx2 - dy2;
        } else {
            drawPixel(x, y, rgb);
            f += dx2;
        }
    }
    drawPixel(p2.x, p2.y, rgb);
}

// FIXME: bounds check
void VideoMode::drawLine(position_t p1, position_t p2, Rgb rgb)
{
    if (p1.x > p2.x) {
        auto tmp = p1;
        p1 = p2;
        p2 = tmp;
    }
    int dx = p2.x - p1.x;
    int dy = p2.y > p1.y ? p2.y - p1.y : p1.y - p2.y;
    if (dx >= dy) octant0(p1, p2, rgb);
    else octant1(p1, p2, rgb);
}

void VideoMode::drawPixel(int x, int y, Rgb rgb)
{
    char* loc = (_base + y * _pitch + x*3);
    *(loc) = rgb.b;
    *(loc + 1) = rgb.g;
    *(loc + 2) = rgb.r;
}

void VideoMode::drawRect(position_t p, int width, int height, Rgb rgb)
{
    width = min(_width - p.x, width);
    height = min(_height - p.y, height);

    auto l = p.x, r = p.x+width-1, t = p.y, b = p.y+height-1;
    drawLine(l, t, r, t, rgb);
    drawLine(l, t, l, b, rgb);
    drawLine(r, t, r, b, rgb);
    drawLine(l, b, r, b, rgb);
}

void VideoMode::drawImage(position_t p, char* data, int width, int height)
{
    width = min(_width - p.x, width);
    height = min(_height - p.y, height);

    char* start = _base + p.y * _pitch + p.x * 3;
    for (int j = 0; j < height; j++) {
        memcpy(start + _pitch*j, data + j*width*3, width*3);
    }
}

void VideoMode::fillRect(position_t p, int width, int height, Rgb rgb)
{
    width = min(_width - p.x, width);
    height = min(_height - p.y, height);

    if (height == 0 || width == 0) return;
    char* start = _base + p.y * _pitch + p.x * 3;
    for (int i = 0; i < width; i++) {
        *(start + i*3) = rgb.b;
        *(start + i*3 + 1) = rgb.g;
        *(start + i*3 + 2) = rgb.r;
    }

    for (int j = 0; j < height-1; j++) {
        memcpy(start+_pitch, start, width*3);
        start += _pitch;
    }
}

void VideoMode::drawString(position_t p, const char* s, Rgb rgb)
{
    int step = builtin_fontinfo.xadvance;
    int len = strlen(s);
    for (int i = 0; i < len; i++) {
        drawChar(p, s[i], rgb);
        p.x += step;
    }
}

void VideoMode::drawChar(position_t p, char c, Rgb rgb)
{
    if (c <= 0) return;
    char* start = _base + p.y * _pitch + p.x * 3;
    const char* f = builtin_font[(int)c-1];

    auto oflags = videolock.lock();
    for (int i = 0; i < 16; i++) {
        char* ln = (start + i*_pitch);
        for (int j = 0; j < 8; j++) {
            auto v = f[i*8+j];
            Rgb* p = (Rgb*)(ln + j*3);
            if (v == '*') {
                *p = rgb;
            } else {
                *p = {0, 0, 0}; //bg color
            }
        }
    }
    videolock.release(oflags);
}

// FIXME: bounds check
void VideoMode::blitCopy(position_t dst, position_t src, int width, int height)
{
    char* start = _base + dst.y * _pitch + dst.x * 3;
    char* old = _base + src.y * _pitch + src.x * 3;
    uint32_t expand = min(width*3, _pitch);
    if (expand == _pitch && dst.x == 0 && src.x == 0) {
        expand *= height;
        memmove(start, old, expand);
        return;
    }
    // slow version
    for (int j = 0; j < height; j++) {
        memcpy(start, old, expand);
        start += _pitch;
        old += _pitch;
    }
}

