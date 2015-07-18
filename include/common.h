#ifndef _COMMON_H
#define _COMMON_H

#include <types.h>
#include <stdarg.h>

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

static inline int max(int a, int b)
{
    return ((a>b)?a:b);
}

static inline int min(int a, int b)
{
    return ((a>b)?b:a);
}



#define SOS_UNUSED __attribute__((unused))

void operator delete(void *ptr);
void* operator new(size_t len);
void operator delete[](void *ptr);
void* operator new[](size_t len);

void kprintf(const char* fmt, ...);
void kvprintf(const char* fmt, va_list args);

void kputs(const char* msg);
void kputchar(char c);

#endif

