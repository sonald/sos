#include <unistd.h>
#include <sprintf.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
 
#define BUF_LEN 128
#define LINE_LEN 1024
static char buf[BUF_LEN], buf2[20];
static char line[LINE_LEN];
static int lp = 0;

static void print_lino(int no)
{
    int len = snprintf(buf2, 19, "%d\t", no);
    write(STDOUT_FILENO, buf2, len);
}

int main(int argc, char* argv[])
{
    int f_lineno = 0;
    
    const char* filepath = NULL;
    if (argc > 1) {
        argc--;
        argv++;

        for (int i = 0; i < argc; i++) {
            auto& av = argv[i];
            if (av[0] && av[0] == '-') {
                switch(av[1]) {
                    case 'n': f_lineno = 1; break;
                }
            } else filepath = av;
        }
    }

    int fd = STDIN_FILENO;
    if (filepath) fd = open(filepath, O_RDONLY, 0);
    if (fd >= 0) {
        int len = 0;
        int no = 1;
        while ((len = read(fd, buf, sizeof buf - 1)) > 0) {
            buf[len] = 0;
            if (f_lineno) {
                int i = 0; 
                while (i < len) {
                    line[lp++] = buf[i];
                    if (buf[i] == '\n') {
                        line[lp] = 0;
                        print_lino(no++);
                        write(STDOUT_FILENO, line, lp);
                        lp = 0;
                    }
                    i++;
                }

            } else write(STDOUT_FILENO, buf, len);
        }

        if (f_lineno && lp) {
            line[lp] = 0;
            print_lino(no++);
            write(STDOUT_FILENO, line, lp);
        }
        if (fd != STDIN_FILENO) close(fd);
    }
    return 0;
}


