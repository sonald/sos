#include <unistd.h>
#include <sprintf.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

static void myprint(const char* buf)
{
    write(STDOUT_FILENO, buf, strlen(buf));
}

#define BUF_SZ 128
char echobuf[128];
 
int main(int argc, char* argv[])
{
    int fd[2];
    pipe(fd);

    sprintf(echobuf, BUF_SZ - 1, "echo: fd0 %d, fd1 %d\n", fd[0], fd[1]);
    myprint(echobuf);

    pid_t pid = fork();
    if (pid == 0) {
        int count = 0;
        // child 
        while(1) {
            sprintf(echobuf, BUF_SZ - 1, "welcome from child #%d", count++);
            write(fd[1], echobuf, strlen(echobuf));
            /*sleep(2000);*/
        }
        
    } else if (pid > 0) {
        while (1) {
            char buf[10];
            int len = read(fd[0], buf, sizeof buf - 1);
            buf[len] = 0;
            myprint("P: ");
            myprint(buf);
            myprint("\n");
            sleep(1000);
        }
    }

    return 0;
}


