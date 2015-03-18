#include "graphics.h"
#include "vm.h"
#include "string.h"

VideoMode videoMode;

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
    vmm.switch_page_directory(vmm.kernel_page_directory());
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
    drawLine(p, {p.x+width, p.y}, rgb);
    drawLine({p.x, p.y+height}, {p.x+width, p.y+height}, rgb);
    drawLine(p, {p.x, p.y+height}, rgb);
    drawLine({p.x+width, p.y}, {p.x+width, p.y+height}, rgb);
}

// FIXME: can be optimized
void VideoMode::fillRect(position_t p, int width, int height, Rgb rgb)
{
    char* start = _base + p.y * _pitch + p.x * 3;
    for (int i = 0; i < width; i++) {
        *(start + i*3) = rgb.b;
        *(start + i*3 + 1) = rgb.g;
        *(start + i*3 + 2) = rgb.r;
    }


    for (int j = 0; j < height; j++) {
        memcpy(start+_pitch, start, width*3);
        start += _pitch;
    }
}

