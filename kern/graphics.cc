#include "graphics.h"
#include "vm.h"
#include "string.h"

VideoMode videoMode;
#define VIDEO_OFFSET(x, y) ((_base) + _pitch * (y) + (x))

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
    // octant 0 now
    int dx = p2.x - p1.x;
    int dy = p2.y - p1.y;
    int dx2 = dx * 2, dy2 = dy * 2;
    int f = dy2 - dx, y = p1.y;

    for (int x = p1.x; x <= p2.x; x++) {
        if (f >= 0) {
            drawPixel(x, ++y, rgb);
            f += dy2 - dx2;
        } else {
            drawPixel(x, y, rgb);
            f += dy2;
        }
    }
}

void VideoMode::drawLine(position_t p1, position_t p2, Rgb rgb)
{
    int dx = p2.x - p1.x;
    int dy = p2.y - p1.y;
    if (p2.x > p1.x && dy < dx) {
        octant0(p1, p2, rgb);
    }
}

void VideoMode::drawPixel(int x, int y, Rgb rgb)
{
    //memcpy(VIDEO_OFFSET(x, y), (char*)&rgb, sizeof rgb);
    *(_base + y * _pitch + x) = rgb.r;
    *(_base + y * _pitch + x + 1) = rgb.g;
    *(_base + y * _pitch + x + 2) = rgb.b;
}

void VideoMode::drawRect(position_t p, int width, int height, Rgb rgb)
{
}

void VideoMode::fillRect(position_t p, int width, int height, Rgb rgb)
{
}

