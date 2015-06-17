#include "display.h"
#include "x86.h"
#include "common.h"
#include "graphics.h"
#include "font.h"
#include <spinlock.h>

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

    auto oflags = _lock.lock();
    if (c == 0x08) { // backspace
        if (_cx > 0) {
            *(vbase + _cy * stride + _cx) = blank;
            _cx--;
        }
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
    _lock.release(oflags);
    set_phy_cursor(_cx, _cy);
}

void Console::set_phy_cursor(int x, int y)
{
    u16 linear = y * 80 + x;

    auto oflags = _lock.lock();
    outb(CRTC_ADDR_REG, CURSOR_LOCATION_HIGH_IND);
    outb(CRTC_ADDR_DATA, linear>>8);
    outb(CRTC_ADDR_REG, CURSOR_LOCATION_LOW_IND);
    outb(CRTC_ADDR_DATA, linear & 0xff);
    _lock.release(oflags);
}

void Console::scroll(int lines) 
{
    if (lines <= 0) return;
    if (lines > 25) lines = 25;

    /* fg: 0, bg: white */
    u8 attrib = (0 << 4) | (0xF & 0x0F);
    u16 blank = (' ') | (attrib << 8);
    int stride = 80;

    auto oflags = _lock.lock();
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
    _lock.release(oflags);
}

Color Console::get_text_color()
{
    return (Color)_attrib;
}

void Console::set_text_color(Color val)
{
    _attrib = (u8)val;
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

    auto oflags = _lock.lock();
    for (int i = 0; i < 25 * stride; i++) {
        *(vbase + i) = blank;
    }    
    _cx = 0, _cy = 0;
    _lock.release(oflags);

    set_phy_cursor(_cx, _cy);
}

void Console::del()
{
    u16 blank = ' ' | (_attrib << 8);
    int stride = 80;

    auto oflags = _lock.lock();
    if (_cx) {
        _cx--;
        *(vbase + _cy * stride + _cx) = blank;
    }
    _lock.release(oflags);

    set_phy_cursor(_cx, _cy);
}

//-----------------------------------------------------------------------------

Color GraphicDisplay::get_text_color()
{
    return _clridx;
}

void GraphicDisplay::set_text_color(Color clr)
{
    auto old = _lock.lock();
    _clridx = (clr > MAXCOLOR) ? WHITE : clr;
    _clr = colormap[_clridx];
    _lock.release(old);
}

void GraphicDisplay::set_cursor(position_t cur)
{
    auto old = _lock.lock();
    _cursor = cur;
    _lock.release(old);
}

position_t GraphicDisplay::get_cursor()
{
    return _cursor;
}

void GraphicDisplay::clear()
{
    auto old = _lock.lock();
    _cursor = {0, 0};
    videoMode.fillRect({0, 0}, videoMode.width(), videoMode.height(),
            {0, 0, 0});
    _lock.release(old);
}

void GraphicDisplay::putchar(char c)
{
    auto& fi = builtin_fontinfo;
    position_t pt = {_cursor.x*fi.xadvance, _cursor.y*fi.yadvance};

    auto old = _lock.lock();
    if (c == 0x08) { // backspace
        if (_cursor.x > 0) {
            videoMode.drawChar(pt, c, {0x0, 0x0, 0x0});
            _cursor.x--;
        }
    } else if (c == 0x09) { // ht
        _cursor.x = min(_cols, (_cursor.x+8) & ~(8-1));
    } else if (c == '\n') {
        _cursor.x = 0;
        _cursor.y++;
    } else if (c == '\r') {
        _cursor.x = 0;
    } else if (c >= ' ') {
        videoMode.drawChar( pt, c, _clr);
        _cursor.x++;
    }

    if (_cursor.x >= _cols) {
        _cursor.x = 0;
        _cursor.y++;
    }

    if (_cursor.y >= _rows)
        scroll(1);
    _lock.release(old);
}

void GraphicDisplay::scroll(int lines) 
{
    if (lines <= 0) return;
    if (lines > _rows) lines = _rows;
    kassert(lines == 1);

    position_t src = {0, builtin_fontinfo.yadvance};
    auto h = (_rows-1) * builtin_fontinfo.yadvance;
    videoMode.blitCopy({0, 0}, src, videoMode.width(), 
            h);

    videoMode.fillRect({0, h}, videoMode.width(),
            videoMode.height() - h, {0, 0, 0});

    _cursor.y = min(max(_cursor.y-1, 0), _rows-1);
}

void GraphicDisplay::del() 
{
    auto& fi = builtin_fontinfo;

    auto old = _lock.lock();
    if (_cursor.x > 0) {
        _cursor.x--;
        position_t pt = {_cursor.x*fi.xadvance, _cursor.y*fi.yadvance};
        videoMode.drawChar(pt, 0x08, {0x0, 0x0, 0x0});
    }
    _lock.release(old);
}

void video_mode_test()
{
    videoMode.drawLine(100, 100, 400, 200, {0xff, 0x00, 0x00});
    videoMode.drawLine(100, 100, 500, 200, {0x00, 0xff, 0x00});

    videoMode.drawLine(0, 150, 590, 150, {0xff, 0xff, 0xff});
    videoMode.drawLine(150, 0, 150, 600, {0xf0, 0xef, 0x8f});

    videoMode.drawLine(200, 100, 300, 400, {0x00, 0xff, 0x00});
    videoMode.drawLine({10, 10}, {200, 50}, {0xe0, 0xff, 0xe0});
    videoMode.drawLine({220, 50}, {12, 12}, {0xe0, 0xff, 0xe0});
    videoMode.drawLine({10, 10}, {80, 300}, {0xe0, 0xff, 0xe0});
    videoMode.drawLine({10, 30}, {100, 30}, {0xe0, 0xff, 0xe0});
    videoMode.drawLine({20, 300}, {200, 60}, {0x20, 0x20, 0xff});
    videoMode.drawLine({20, 100}, {200, 60}, {0x20, 0x20, 0xff});
    videoMode.drawLine({20, 100}, {200, 70}, {0x20, 0x20, 0xff});
    videoMode.drawLine({20, 100}, {100, 90}, {0x20, 0x20, 0xff});
    videoMode.drawLine({40, 230}, {120, 40}, {0xff, 0xff, 0x00});
    videoMode.drawLine({150, 200}, {150, 40},  {0xf0, 0x20, 0xf0});
    videoMode.drawLine({200, 200}, {500, 500}, {0xf0, 0x20, 0xf0});
    videoMode.drawLine({200, 500}, {500, 200}, {0xf0, 0x20, 0xf0});

    videoMode.fillRect({40, 40}, 40, 20, colormap[CYAN]);
    videoMode.drawRect({38, 38}, 44, 24, {0xff, 0xe0, 0x00});

    videoMode.blitCopy({100, 100}, {38, 38}, 44, 24);
    videoMode.drawString({80, 80}, "hello, SOS", {0xe0, 0x80, 0xe0});
    videoMode.fillRect({0, 570}, 800, 40, {0xff, 0x00, 0xff});

    videoMode.blitCopy({0, 400}, {0, 560}, 800, 40);

    for (int i = 0; i <= MAXCOLOR; i++) {
        current_display->set_text_color((Color)(i % (MAXCOLOR + 1)));
        kprintf("%d: comming here  %x \n", i, i);
    }
}

