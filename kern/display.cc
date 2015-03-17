#include "display.h"
#include "x86.h"
#include "common.h"
#include "graphics.h"

Console console;
GraphicDisplay graph_console;
Display* current_display = &console;

static const u16 CRTC_ADDR_REG = 0x3D4;
static const u16 CRTC_ADDR_DATA = 0x3D5;

static const u8 CURSOR_LOCATION_HIGH_IND = 0x0E;
static const u8 CURSOR_LOCATION_LOW_IND = 0x0F;
static volatile u16* vbase = (u16*)0xC00B8000;

void Console::putchar(char c)
{
    u16 ch = c | (_attrib << 8);
    u16 blank = ' ' | (_attrib << 8);
    int stride = 80;

    if (c == 0x08) { // backspace
        if (_cx == 0) return;
        *(vbase + _cy * stride + _cx) = blank;
        _cx--;
    } else if (c == 0x09) { // ht
        _cx = min(stride, (_cx+8) & ~(8-1));
    } else if (c == '\n') {
        _cx = 0;
        _cy++;
    } else if (c == '\r') {
        _cx = 0;
    } else if (c >= ' ') {
        *(vbase + _cy * stride + _cx) = ch;
        _cx++;
    }

    if (_cx >= stride) {
        _cx = 0;
        _cy++;
    }

    scroll(_cy-24);
    set_phy_cursor(_cx, _cy);
}

void Console::set_phy_cursor(int x, int y)
{
    u16 linear = y * 80 + x;
    outb(CRTC_ADDR_REG, CURSOR_LOCATION_HIGH_IND);
    outb(CRTC_ADDR_DATA, linear>>8);
    outb(CRTC_ADDR_REG, CURSOR_LOCATION_LOW_IND);
    outb(CRTC_ADDR_DATA, linear & 0xff);
}

void Console::scroll(int lines) 
{
    if (lines <= 0) return;
    if (lines > 25) lines = 25;

    /* fg: 0, bg: white */
    u8 attrib = (0 << 4) | (0xF & 0x0F);
    u16 blank = (' ') | (attrib << 8);
    int stride = 80;

    for (int i = lines; i < 25; i++) {
        int dst = (i-lines) * stride, src = i * stride;
        for (int j = 0; j < stride; ++j) {
            *(vbase+dst+j) = *(vbase+src+j);
        }
    }    

    for (int i = (25-lines) * stride; i < 25 * stride; i++) {
        *(vbase + i) = blank;
    }

    _cy = max(_cy-lines, 0);
}

u8 Console::get_text_color()
{
    return _attrib;
}

void Console::set_text_color(u8 val)
{
    _attrib = val;
}

void Console::set_cursor(position_t cur)
{
    _cx = max(min(cur.x, 79), 0);
    _cy = max(min(cur.y, 24), 0);
    set_phy_cursor(_cx, _cy);
}

position_t Console::get_cursor()
{
    return {_cx, _cy};
}

void Console::clear()
{
    u8 attrib = (0 << 4) | (0xF & 0x0F);
    u16 blank = (' ') | (attrib << 8);
    int stride = 80;

    for (int i = 0; i < 25 * stride; i++) {
        *(vbase + i) = blank;
    }    
    _cx = 0, _cy = 0;
    set_phy_cursor(_cx, _cy);
}


u8 GraphicDisplay::get_text_color()
{
}

void GraphicDisplay::set_text_color(u8 clr)
{

}

void GraphicDisplay::set_cursor(position_t cur)
{
}

position_t GraphicDisplay::get_cursor()
{
}

void GraphicDisplay::clear()
{
}

void GraphicDisplay::putchar(char c)
{
}

