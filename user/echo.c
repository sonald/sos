#include <unistd.h>
#include <sprintf.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

char buf[32] = "";
int logo;
char undef[128] = "inited string";

static void myprint(const char* buf)
{
    write(0, buf, strlen(buf));
}

static void test_fs_readdir(const char* dir)
{
    int fd = open(dir, O_RDONLY, 0);
    if (fd >= 0) {
        struct dirent dire;
        while (readdir(fd, &dire, 1) > 0) {
            char buf[64] = "";
            int len = sprintf(buf, sizeof buf - 1, "%s, ", dire.d_name);
            write(0, buf, len);
        }
        close(fd);
    }
}

static void test_fs_read(const char* filepath)
{
    int fd = open(filepath, O_RDONLY, 0);
    if (fd >= 0) {
        char buf[64];
        int len = 0;
        while ((len = read(fd, buf, sizeof buf - 1)) > 0) {
            buf[len] = 0;
            write(0, buf, len);
        }
        close(fd);
    }
}

void do_fs_test()
{
    myprint("in to do_fs_test   ");
    test_fs_readdir("/boot/grub/");
    // test_fs_readdir("/ram");

    char filename[] = "/boot/grub/grub.cfg";
    // char filename[NAMELEN+1] = "/longnamedmsdosfile.txt";
    // test_fs_read(filename);
}

#ifdef __cplusplus
extern "C"
#endif

void _start()
{
    logo = 'C';
    pid_t pid = fork();
    if (pid == 0) {
        logo = 'D';
        myprint(undef);
        do_fs_test();

        pid = fork();
        pid = getpid();
        int step = 0;
        for(;;) {
            int len = sprintf(buf, sizeof buf - 1, "{%c%d %d} ", logo, pid, step);
            write(0, buf, len);

            sleep(2000);
            step++;
        }
    } else if (pid > 0) {
        logo = 'B';
        pid = getpid();
        int len = sprintf(buf, sizeof buf - 1, "{%c%d going to exit} ", logo, pid);
        write(0, buf, len);

        exit();
        // for(;;) sleep(500);
    }
}


