#include "syscall.h"

static pid_t sys_getpid()
{
    pid_t pid;
    asm volatile ( "int $0x80 \n"
            :"=a"(pid)
            :"a"(SYS_getpid)
            :"cc", "memory");
    return pid;
}

static pid_t sys_fork()
{
    pid_t old = sys_getpid(), pid;
    asm volatile ( "int $0x80 \n"
            :"=a"(pid)
            :"a"(SYS_fork)
            :"cc", "memory");

    pid_t now = sys_getpid();
    if (old == now) return pid;
    else return 0;
}

static int sys_write(int fd, const void* buf, size_t nbyte)
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
    int logo = 'C';
    char buf[] = "C";

    pid_t pid = sys_fork();
    if (pid == 0) {
        logo = 'D';
    } else {
        logo = 'E';
    }

    u8 step = 0;
    for(;;) {
        buf[0] = logo;
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
