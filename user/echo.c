#include <unistd.h>

_syscall0(pid_t, getpid);
_syscall0(pid_t, fork);
_syscall3(int, write, int, fd, const void*, buf, size_t, nbyte);

extern "C" void _start()
{
    int logo = 'C';
    char buf[] = "C";

    pid_t pid = fork();
    if (pid == 0) {
        logo = 'D';
    } 
    pid = getpid();

    u8 step = 0;
    for(;;) {
        buf[0] = logo;
        write(0, buf, 1);
        buf[0] = '0'+pid;
        write(0, buf, 1);
        buf[0] = '0'+(step%10);
        write(0, buf, 1);

        volatile int r = 0;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 0x7fffff; ++j) {
                r += j;
            }
        }

        step++;
    }
}
