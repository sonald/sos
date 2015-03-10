#ifndef _COMMON_H
#define _COMMON_H 

#include "types.h"
#include "stdarg.h"

#if !defined __i386__
#error "target error, need a cross-compiler with ix86-elf as target"
#endif

void panic(const char* fmt, ...);
#define kassert(cond) do { \
    if (!(cond)) { \
        kprintf("[%s:%d]: ", __func__, __LINE__); \
        panic("assert " #cond " failed");  \
    } \
} while(0)

#define ARRAYLEN(arr) (sizeof(arr)/sizeof(arr[0]))

int max(int a, int b);
int min(int a, int b);


#define SOS_UNUSED __attribute__((unused))

void operator delete(void *ptr);
void* operator new(size_t len);
void operator delete[](void *ptr);
void* operator new[](size_t len);

/*
 * early vga access
 */

// all theses routines assume in VGA text-mode, which is guaranteed by GRUB
void kprintf(const char* fmt, ...);
void kvprintf(const char* fmt, va_list args);

void kputs(const char* msg);
void kputchar(char c);


#define BLACK   0   // BLACK   
#define BLUE    1   // BLUE    
#define GREEN   2   // GREEN   
#define CYAN    3   // CYAN    
#define RED     4   // RED     
#define MAGENTA 5   // MAGENTA 
#define BROWN   6   // BROWN   
#define LIGHT_GREY  7   // LIGHT GREY 
#define DARK_GREY   8   // DARK GREY
#define LIGHT_BLUE  9   // LIGHT BLUE
#define LIGHT_GREEN 10  // LIGHT GREEN
#define LIGHT_CYAN  11  // LIGHT CYAN
#define LIGHT_RED   12  // LIGHT RED
#define LIGHT_MAGENTA   13  // LIGHT MAGENTA
#define LIGHT_BROWN 14  // LIGHT BROWN
#define WHITE  15  // WHITE


#define CURSOR(x, y) (((x << 8) & 0xff00) | (y & 0x00ff))
#define CURSORX(cur) (((cur) >> 8) & 0x00ff)
#define CURSORY(cur) ((cur) & 0x00ff)
#define COLOR(fg, bg) ((((fg) & 0x0f) | ((bg) & 0xf0)) & 0xff)

u8 get_text_color();
void set_text_color(u8);
void set_cursor(u16 cur);
u16 get_cursor();
void clear();

#endif


