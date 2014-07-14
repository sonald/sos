#include "common.h"
#include "sprintf.h"

static const u16 CRTC_ADDR_REG = 0x3D4;
static const u16 CRTC_ADDR_DATA = 0x3D5;

static const u8 CURSOR_LOCATION_HIGH_IND = 0x0E;
static const u8 CURSOR_LOCATION_LOW_IND = 0x0F;

static volatile u16* vbase = (u16*)0xC00B8000;
static int cx = 0, cy = 0;
static u8 _attrib = 0x0F;

inline int max(int a, int b)
{
    return ((a>b)?a:b); // this'll crash, why?
}

inline int min(int a, int b)
{
    return ((a>b)?b:a);
}

void outb(u16 port, u8 val)
{
    __asm__ __volatile__ ( "outb %1, %0" : : "dN"(port), "a"(val));
}

u8 inb(u16 port)
{
    u8 val;
    __asm__ __volatile__ ( "inb %1, %0" : "=a"(val) : "dN"(port));
    return val;
}

u16 inw(u16 port)
{
    u16 val;
    __asm__ __volatile__ ( "inw %1, %0" : "=a"(val) : "dN"(port));
    return val;
}


static void set_phy_cursor(int x, int y)
{
    u16 linear = y * 80 + x;
    outb(CRTC_ADDR_REG, CURSOR_LOCATION_HIGH_IND);
    outb(CRTC_ADDR_DATA, linear>>8);
    outb(CRTC_ADDR_REG, CURSOR_LOCATION_LOW_IND);
    outb(CRTC_ADDR_DATA, linear & 0xff);
}

static void scroll(int lines) 
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

    cy = max(cy-lines, 0);
}

void kprintf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    kvprintf(fmt, args);
    va_end(args);
}

void kvprintf(const char* fmt, va_list args)
{
    int d = 0;
    u32 u = 0;
    char* s = NULL;
    char c = ' ';
    char buf[32];

    while (*fmt) {
        char ch = *fmt;
        if (ch == '%') {
            switch(*++fmt) {
                case 'b': case 'B':
                    d = va_arg(args, int);
                    kputs(itoa(d, buf, 2));
                    break;

                case 'x': case 'X':
                    u = va_arg(args, u32);
                    kputs(utoa(u, buf, 16));
                    break;

                case 'd': 
                    d = va_arg(args, int);
                    kputs(itoa(d, buf, 10));
                    break;

                case 'u':
                    u = va_arg(args, u32);
                    kputs(utoa(u, buf, 10));
                    break;
                    
                case '%':
                    kputchar('%');
                    break;

                case 'c':
                    c = va_arg(args, char);
                    kputchar(c);
                    break;

                case 's':
                    s = va_arg(args, char*);
                    kputs(s?s:"(NULL)");
                    break;

                default:
                    break;
            }
        } else {
            kputchar(ch);
        }
        fmt++;
    }
}

void kputchar(char c)
{
    u16 ch = c | (_attrib << 8);
    u16 blank = ' ' | (_attrib << 8);
    int stride = 80;

    if (c == 0x08) { // backspace
        if (cx == 0) return;
        *(vbase + cy * stride + cx) = blank;
        cx--;
    } else if (c == 0x09) { // ht
        cx = min(stride, (cx+8) & ~(8-1));
    } else if (c == '\n') {
        cx = 0;
        cy++;
    } else if (c == '\r') {
        cx = 0;
    } else if (c >= ' ') {
        *(vbase + cy * stride + cx) = ch;
        cx++;
    }

    if (cx >= stride) {
        cx = 0;
        cy++;
    }

    scroll(cy-24);
    set_phy_cursor(cx, cy);
}

void kputs(const char* msg)
{
    const char* p = msg;
    while (*p) {
        kputchar(*p);
        p++;
    }
}

void set_text_color(u8 fg, u8 bg)
{
    _attrib = ((fg & 0x0f) | (bg & 0xf0)) & 0xff;
}

void set_cursor(u16 cur)
{
    cx = max(min(CURSORX(cur), 79), 0);
    cy = max(min(CURSORY(cur), 24), 0);
    set_phy_cursor(cx, cy);
}

u16 get_cursor()
{
    return CURSOR(cx, cy);
}

void clear()
{
    u8 attrib = (0 << 4) | (0xF & 0x0F);
    u16 blank = (' ') | (attrib << 8);
    int stride = 80;

    for (int i = 0; i < 25 * stride; i++) {
        *(vbase + i) = blank;
    }    
    cx = 0, cy = 0;
    set_phy_cursor(cx, cy);
}

void panic(const char* fmt, ...)
{
    asm ("cli");
    va_list args;
    va_start(args, fmt);
    kvprintf(fmt, args);
    va_end(args);
    for(;;) {
        asm volatile ("hlt");
    }
}


