#include <unistd.h>
#include <sprintf.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <lru.h>
#include <stat.h>
#include <sos/signal.h>

static volatile int quit = 0;
static void signal_handler(int sig)
{
    pid_t pid = getpid();
    printf("%s: pid %d, sig %d\n", __func__, pid, sig);
    if (sig == SIGUSR1) {
        /*kill(pid, SIGUSR2);*/
    }
    if (sig == SIGUSR2)
        quit = 1;
}

int main(int argc, char *argv[])
{
    signal(SIGUSR1, (uint32_t)signal_handler);
    signal(SIGUSR2, (uint32_t)signal_handler);

    pid_t pid = fork();
    if (pid == 0) {
        printf("child(%d): busy loop\n", getpid());
        int count = 0;
        while (!quit) {
            sleep(1000);
            printf("loop %d\n", count++);
            sleep(1000);
            kill(getppid(), SIGUSR1);
        }

    } else if (pid > 0) {
        sleep(100);
        kill(pid, SIGUSR1);
        sigset_t mask = 0;
        sigsuspend(&mask);
        printf("wake up from suspend\n");
        kill(pid, SIGUSR2);
        wait();
        printf("parent(%d) quit\n", getpid());
    }

    return 0;
}
