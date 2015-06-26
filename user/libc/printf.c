#include "printf.h"
#include "sprintf.h"
#include <types.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

static int puts(const char* buf)
{
    return write(STDOUT_FILENO, buf, strlen(buf));
}

int vprintf(const char* fmt, va_list args)
{
    int d = 0;
    u32 u = 0;
    char* s = NULL;
    char c = ' ';
    char buf[32];
    int ret = 0;

    while (*fmt) {
        char ch = *fmt;
        if (ch == '%') {
            switch(*++fmt) {
                case 'b': case 'B':
                    d = va_arg(args, int);
                    ret += puts(itoa(d, buf, 2));
                    break;

                case 'x': case 'X':
                    u = va_arg(args, u32);
                    ret += puts(utoa(u, buf, 16));
                    break;

                case 'd':
                    d = va_arg(args, int);
                    ret += puts(itoa(d, buf, 10));
                    break;

                case 'u':
                    u = va_arg(args, u32);
                    ret += puts(utoa(u, buf, 10));
                    break;

                case '%':
                    ret += puts("%");
                    break;

                case 'c':
                    c = va_arg(args, int);
                    buf[0] = c;
                    buf[1] = 0;
                    ret += puts(buf);
                    break;

                case 's':
                    s = va_arg(args, char*);
                    ret += puts(s?s:"(NULL)");
                    break;

                default:
                    break;
            }
        } else {
            buf[0] = ch;
            buf[1] = 0;
            ret += puts(buf);
        }
        fmt++;
    }

    return ret;
}

int printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int nwrite = vprintf(fmt, args);
    va_end(args);
    return nwrite;
}

