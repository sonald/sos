#include <unistd.h>
#include <sprintf.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <stat.h>

#define BUF_SIZE 1024
static const char* base = NULL;

static inline int min(int a, int b) 
{
    return a < b ? a : b;
}

static inline int max(int a, int b) 
{
    return a > b ? a : b;
}

static int f_long = 0, f_single = 0, f_stream = 0;

static char* mode_to_str(mode_t m, char* buf)
{
    if (S_ISDIR(m)) buf[0] = 'd';
    if (m & S_IRUSR) buf[1] = 'r';
    if (m & S_IWUSR) buf[2] = 'w';
    if (m & S_IXUSR) buf[3] = 'x';
    if (m & S_IRGRP) buf[4] = 'r';
    if (m & S_IWGRP) buf[5] = 'w';
    if (m & S_IXGRP) buf[6] = 'x';
    if (m & S_IROTH) buf[7] = 'r';
    if (m & S_IWOTH) buf[8] = 'w';
    if (m & S_IXOTH) buf[9] = 'x';
    return buf;
}

static void do_stream(int fd)
{
    struct dirent de;
    if (readdir(fd, &de, 1) > 0) {
        printf("%s", de.d_name);
    }
    while (readdir(fd, &de, 1) > 0) {
        printf(", %s", de.d_name);
    }
    printf("\n");
}

static void do_single(int fd)
{
    struct dirent de;
    while (readdir(fd, &de, 1) > 0) {
        printf("%s\n", de.d_name);
    }
}

static void do_normal(int fd)
{
    int count = 0; 
    int maxcol = 0;
    struct dirent de;
    while (readdir(fd, &de, 1) > 0) {
        maxcol = max(maxcol, strlen(de.d_name));
    }
    maxcol = min(maxcol, 80);

    int ln_max = 80 / maxcol;
    char* buf = (char*)malloc(maxcol+1);
    buf[maxcol] = 0;

    lseek(fd, 0, SEEK_SET);
    while (readdir(fd, &de, 1) > 0) {
        memset(buf, ' ', maxcol);
        sprintf(buf, "%s", de.d_name);
        printf("%s", buf);
        if (++count % ln_max == 0) printf("\n");
    }
    printf("\n");
    free(buf);
}

static void do_long_mode(int fd)
{
    struct dirent de;
    while (readdir(fd, &de, 1) > 0) {
        char modes[] = "----------";
        struct stat stbuf;
        char name[NAMELEN+1];
        snprintf(name, NAMELEN, "%s/%s", base, de.d_name);
        if (lstat(name, &stbuf) == 0) {
            printf("%s\t%d\t%d\t%d\t%d\t%s\n",
                    mode_to_str(stbuf.st_mode, modes), 
                    stbuf.st_uid, stbuf.st_gid,
                    stbuf.st_size, stbuf.st_mtime, de.d_name);
        }
    }
}

int main(int argc, char* const argv[])
{
    base = "/";
    if (argc > 1) {
        argc--;
        argv++;

        for (int i = 0; i < argc; i++) {
            auto& av = argv[i];
            if (av[0] && av[0] == '-') {
                switch(av[1]) {
                    case 'l': f_long = 1; break;
                    case '1': f_single = 1; break;
                    case 'm': f_stream = 1; break;
                }
            } else base = av;
        }
    }

    int fd = open(base, O_RDONLY, 0);
    if (fd >= 0) {
        if (f_stream) do_stream(fd);
        else if (f_long) do_long_mode(fd);
        else if (f_single) do_single(fd);
        else do_normal(fd); 
        close(fd);
    }
    return 0;
}

