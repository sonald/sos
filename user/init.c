#include <unistd.h>
#include <sprintf.h>
#include <fcntl.h>

#define NAME_MAX 128

static const char* prompt = "> ";
static char cmd_name[NAME_MAX] = "/echo";

#ifdef __cplusplus
extern "C"
#endif
void _start()
{
    char buf[32] = "";

    pid_t pid = fork();
    if (pid == 0) {
        exec(cmd_name, 0, 0);
    } else {
        pid = getpid();
        for(;;) {
            int len = sprintf(buf, sizeof buf - 1, "[A%d waiting]  ", pid);
            write(0, buf, len);

            pid_t cpid = wait();

            len = sprintf(buf, sizeof buf - 1, "[A%d child %d exit]  ", pid, cpid);
            write(0, buf, len);
        }
    }
}
