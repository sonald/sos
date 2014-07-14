#include "sprintf.h"
#include "types.h"
#include "stdarg.h"

char* itoa(int d, char* buf, int base)
{
    if (base < 2 || base > 36) {
        *buf = '\0';
        return buf;
    }
    
    const char map[] = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";
    char* p = buf, *dp = buf;
    if (d < 0 && base == 10) {
        *buf++ = '-';
        dp = buf;
    }   
    
     do {
         *buf++ = map[35 + d % base];
         d /= base;
     } while (d);
     *buf-- = '\0';
     
     while (dp < buf) {
         char c = *dp;
         *dp++ = *buf;
         *buf-- = c;
     }
     
     return p;
}

char* utoa(u32 u, char* buf, int base) 
{
    if (base < 2 || base > 36) {
        *buf = '\0';
        return buf;
    }
    
    const char map[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char* p = buf, *dp = buf;
    
     do {
         *buf++ = map[u % base];
         u /= base;
     } while (u);
     *buf-- = '\0';
     
     while (dp < buf) {
         char c = *dp;
         *dp++ = *buf;
         *buf-- = c;
     }
     
     return p;        
}

static char* ksputs(char* buf, size_t* len, const char* p)
{
    while (*p && (*len)--) {
        *buf++ = *p++;
    }
    return buf;
}

int vsprintf(char* sbuf, size_t len, const char* fmt, va_list args)
{
    char* old = sbuf;

    int d = 0;
    u32 u = 0;
    char* s = NULL;
    char c = ' ';
    char buf[32];

    while (*fmt && len) {
        char ch = *fmt;
        if (ch == '%') {
            switch(*++fmt) {
                case 'b': case 'B':
                    d = va_arg(args, int);
                    sbuf = ksputs(sbuf, &len, itoa(d, buf, 2));
                    break;

                case 'x': case 'X':
                    u = va_arg(args, u32);
                    sbuf = ksputs(sbuf, &len, utoa(u, buf, 16));
                    break;

                case 'd': 
                    d = va_arg(args, int);
                    sbuf = ksputs(sbuf, &len, itoa(d, buf, 10));
                    break;

                case 'u':
                    u = va_arg(args, u32);
                    sbuf = ksputs(sbuf, &len, utoa(u, buf, 10));
                    len--;
                    break;
                    
                case '%':
                    *sbuf++ = '%';
                    len--;
                    break;

                case 'c':
                    c = va_arg(args, int);
                    *sbuf++ = c;
                    len--;
                    break;

                case 's':
                    s = va_arg(args, char*);
                    sbuf = ksputs(sbuf, &len, s?s:"(NULL)");
                    break;

                default:
                    break;
            }
        } else {
            *sbuf++ = ch;
        }
        fmt++;
    }

    return sbuf - old;
}

int sprintf(char* buf, size_t len, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int nwrite = vsprintf(buf, len, fmt, args);
    va_end(args);
    return nwrite;
}

