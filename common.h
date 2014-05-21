#ifndef _COMMON_H
#define _COMMON_H 

typedef unsigned int u32;
typedef int s32;
typedef unsigned short u16;
typedef short s16;
typedef unsigned char u8;
typedef char s8;

#ifndef NULL
#define NULL 0
#endif

#if 0 /* buggy */
#ifndef max
#define max(a, b) ({ \
        typeof(a) _a = (a); \
        typeof(b) _b = (b); \
        (_a > _b) ? _a: _b; \
        })
#define min(a, b) max(b, a)
#endif
#endif

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

// use internal static buf, not thread-safe, do no free it
char* itoa(u32 d, int base);

// early vga access
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

#define CURSOR(x, y) (((x << 8) & 0xff00) | (y & 0x00ff))
#define CURSORX(cur) (((cur) >> 8) & 0x00ff)
#define CURSORY(cur) ((cur) & 0x00ff)

void set_cursor(u16 cur);
u16 get_cursor();
void clear();

#endif


