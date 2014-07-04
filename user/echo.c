#include "syscall.h"

int sys_write(char c)
{
    int ret;
    asm volatile ( "int $0x80 \n"
            :"=a"(ret)
            :"a"(SYS_write), "b"(c)
            :"cc", "memory");
    return ret;
}

extern "C" void _start()
{
    u8 step = 0;
    for(;;) {
        int ret = sys_write('C');
        ret = ret % 10;
        sys_write('0'+ret);
        sys_write('0'+(step%10));
        sys_write(' ');

        volatile int r = 0;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 0x7fffff; ++j) {
                r += j;
            }
        }

        step++;
    }
}
