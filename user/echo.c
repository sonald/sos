#include "syscall.h"

int sys_write(int fd, const void* buf, size_t nbyte)
{
    int ret;
    asm volatile ( "int $0x80 \n"
            :"=a"(ret)
            :"a"(SYS_write), "b"(fd), "c"(buf), "d"(nbyte)
            :"cc", "memory");
    return ret;
}

extern "C" void _start()
{
    u8 step = 0;
    for(;;) {
        char buf[] = "C";
        int ret = sys_write(0, buf, 1);
        ret = ret % 10;
        buf[0] = '0'+ret;
        sys_write(0, buf, 1);
        buf[0] = '0'+(step%10);
        sys_write(0, buf, 1);
        buf[0] = ' ';
        sys_write(0, buf, 1);

        volatile int r = 0;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 0x7fffff; ++j) {
                r += j;
            }
        }

        step++;
    }
}
