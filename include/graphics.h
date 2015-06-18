#ifndef _SOS_GRAPHICS_H
#define _SOS_GRAPHICS_H 

#include "types.h"

typedef struct VbeInfoBlock_s {
    char VbeSignature[4];             // == "VESA"
    uint16_t VbeVersion;                 // == 0x0300 for VBE 3.0
    uint16_t OemStringPtr[2];            // isa vbeFarPtr
    uint8_t Capabilities[4];
    uint16_t VideoModePtr[2];         // isa vbeFarPtr
    uint16_t TotalMemory;             // as # of 64KB blocks
} __attribute__((packed)) VbeInfoBlock_t;
 
typedef struct ModeInfoBlock_s {
    uint16_t attributes;
    uint8_t winA,winB;
    uint16_t granularity;
    uint16_t winsize;
    uint16_t segmentA, segmentB;
    uint32_t realFctPtr;
    uint16_t pitch; // bytes per scanline

    uint16_t Xres, Yres;
    uint8_t Wchar, Ychar, planes, bpp, banks;
    uint8_t memory_model, bank_size, image_pages;
    uint8_t reserved0;

    uint8_t red_mask, red_position;
    uint8_t green_mask, green_position;
    uint8_t blue_mask, blue_position;
    uint8_t rsv_mask, rsv_position;
    uint8_t directcolor_attributes;

    uint32_t physbase;  // your LFB (Linear Framebuffer) address ;)
    uint32_t reserved1;
    uint16_t reserved2;
} __attribute__((packed)) ModeInfoBlock_t;

struct Rgb 
{
    uint8_t b, g, r; // match memory layout of pixel
    Rgb(): b{0}, g{0}, r{0} {}
    Rgb(uint8_t r, uint8_t g, uint8_t b): b{b}, g{g}, r{r} {}
    Rgb(uint32_t rgb)
        :b{(uint8_t)(rgb & 0xff)},
        g{(uint8_t)((rgb>>8) & 0xff)}, r{(uint8_t)(rgb>>16)} {}
};

typedef struct position_s
{
    int x, y;
} position_t;

class VideoMode
{
    public:
        void init(ModeInfoBlock_t* modeinfo);
        void drawLine(int x0, int y0, int x1, int y1, Rgb rgb);
        void drawLine(position_t p1, position_t p2, Rgb rgb);
        void drawPixel(int x, int y, Rgb rgb);
        void drawRect(position_t p, int width, int height, Rgb rgb);
        void fillRect(position_t p, int width, int height, Rgb rgb);
        void drawString(position_t p, const char* s, Rgb rgb);
        void drawChar(position_t p, char c, Rgb rgb);
        void drawImage(position_t p, char* data, int width, int height);

        void blitCopy(position_t dst, position_t src, int width, int height);

        int width() const { return _width; }
        int height() const { return _height; }

    private:
        char* _base;
        uint32_t _pitch;
        uint32_t _width, _height;

        void octant0(position_t p1, position_t p2, Rgb rgb);
        void octant1(position_t p1, position_t p2, Rgb rgb);
};

extern VideoMode videoMode;
extern Rgb colormap[];

#endif
