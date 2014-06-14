#ifndef _COMMON_H
#define _COMMON_H 

#if !defined __i386__
#error "target error, need a cross-compiler with ix86-elf as target"
#endif

#include <stddef.h>
#if !defined __cplusplus
#include <stdbool.h>
#endif
#include <stdint.h>

#if defined __cplusplus

#define BEGIN_CDECL extern "C" {
#define END_CDECL }

#else

#define BEGIN_CDECL 
#define END_CDECL

#endif

#if !defined uint32_t
typedef unsigned int u32;
typedef int s32;
typedef unsigned short u16;
typedef short s16;
typedef unsigned char u8;
typedef char s8;
#else
typedef uint32_t u32;
typedef int32_t s32;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint8_t u8;
typedef int8_t s8;
#endif

#ifndef NULL
#define NULL (void*)0
#endif

extern u32 kernel_virtual_base;

int max(int a, int b);
int min(int a, int b);

void* memcpy(void* dst, const void* src, int n);
void* memset(void *b, int c, int len);
int strlen(const char* s);
char* strcpy(char* dst, const char* src);

// port io
void outb(u16 port, u8 val);
u8 inb(u16 port);
u16 inw(u16 port);

char* itoa(int d, char* buf, int base);
char* utoa(u32 u, char* buf, int base);

/*
 * early vga access
 */

// all theses routines assume in VGA text-mode, which is guaranteed by GRUB
void kprintf(const char* fmt, ...);

typedef char* va_list;
#define va_start(ap, last) (ap = (((char*)&last) + sizeof(last)))
#define va_arg(ap, type) ({      \
        type r = *(type*)ap;     \
        ap = ap + sizeof(type*); \
        r;                       \
        }) 

#define va_end(ap) (ap = NULL)

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

void set_text_color(u8 fg, u8 bg);
void set_cursor(u16 cur);
u16 get_cursor();
void clear();

#endif


