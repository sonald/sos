#include "syscall.h"

void task2()
{
    u8 step = 0;
    for(;;) {
        int ret = 0;
        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"('C')
                :"cc", "memory");
        ret = ret % 10;
        asm volatile ( "int $0x80"::"a"(SYS_write), "b"('0'+ret));

        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"('0'+ (step%10))
                :"cc", "memory");

        asm volatile ( "int $0x80" ::"a"(SYS_write), "b"(' ') :"cc", "memory");

        volatile int r = 0;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 0x7fffff; ++j) {
                r += j;
            }
        }

        step++;
    }
}
