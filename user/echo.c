#include <unistd.h>
#include <sprintf.h>
#include <string.h>
#include <fcntl.h>

char buf[32] = "";
int logo;
char undef[128] = "inited string";

#ifdef __cplusplus
extern "C"
#endif

void _start()
{
    logo = 'C';
    pid_t pid = fork();
    if (pid == 0) {
        logo = 'D';
        write(0, undef, strlen(undef));

        pid = getpid();

        int step = 0;
        for(;;) {
            int len = sprintf(buf, sizeof buf - 1, "{%c%d %d} ", logo, pid, step);
            write(0, buf, len);

            sleep(2000);
            step++;
        }
    } else if (pid > 0) {
        sleep(3000);
        pid = getpid();

        int len = sprintf(buf, sizeof buf - 1, "{%c%d going to exit} ", logo, pid);
        write(0, buf, len);

        exit();
    }
}

