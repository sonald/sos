#include "unistd.h"
#include "sprintf.h"

_syscall0(pid_t, getpid);
_syscall0(pid_t, fork);
_syscall3(int, write, int, fd, const void*, buf, size_t, nbyte);

#ifdef __cplusplus
extern "C" 
#endif
void _start()
{
    int logo = 'C';
    char buf[] = "this is the initial content of buffer";

    pid_t pid = fork();
    if (pid == 0) {
        logo = 'D';
    } 
    pid = getpid();

    int step = 0;
    for(;;) {
        int len = sprintf(buf, sizeof buf - 1, "{%c%d %d} ", logo, pid, step);
        write(0, buf, len);

        volatile int r = 0;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 0x7fffff; ++j) {
                r += j;
            }
        }

        step++;
    }
}
