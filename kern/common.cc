#include "common.h"
#include "sprintf.h"
#include "display.h"

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

void kputs(const char* msg)
{
    const char* p = msg;
    while (*p) {
        kputchar(*p);
        p++;
    }
}
void kputchar(char c)
{
    current_display->putchar(c);
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


