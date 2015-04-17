#include <unistd.h>
#include <sprintf.h>
#include <fcntl.h>

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

        sleep(3000);
        step++;
    }
}
