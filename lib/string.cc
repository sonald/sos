#include "string.h"

//dst and src should not overlay
//FIMXE: optimize when needed
void* memcpy(void* dst, const void* src, int n)
{
    u8* p = (u8*)dst;
    const u8* q = (u8*)src;
    for (int i = 0; i < n; i++) {
        *(p+i) = *(q+i);
    }
    return dst;
}

void* memset(void *dst, int c, int len)
{
    u8* p = (u8*)dst;
    for (int i = 0; i < len; i++) {
        *(p+i) = (u8)c;
    }
    return dst;
}

int strlen(const char* s)
{
    int len = 0;
    while (*(s+len)) len++;
    return len;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && *s2) {
        if (*s1 > *s2) return -1;
        else if (*s1 < *s2) return 1;
        s1++;
        s2++;
    }

    if (!*s1 && *s2) return 1;
    else if (*s1 && !*s2) return -1;
    return 0;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    while (*s1 && *s2 && n) {
        if (*s1 > *s2) return -1;
        else if (*s1 < *s2) return 1;
        s1++;
        s2++;
        n--;
    }

    if (!*s1 && *s2) return 1;
    else if (*s1 && !*s2) return -1;
    return 0;
}

char* strcpy(char* dst, const char* src)
{
    char* d = dst;
    while (*src) {
        *dst++ = *src++;
    }
    *dst = '\0';
    return d;
}

