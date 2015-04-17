#include <unistd.h>
#include <sprintf.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" 
#endif

void _start()
{
    char cmd_name[] = "/echo";
    char buf[32] = "";

    pid_t pid = fork();
    if (pid == 0) { 
        exec(cmd_name, 0, 0);
    } else {
        pid = getpid();
        int step = 0;
        for(;;) {
            int len = sprintf(buf, sizeof buf - 1, "[A%d %d]", pid, step);
            write(0, buf, len);

            sleep(5000);
            step++;
        }
    }
}
