#include <unistd.h>
#include <sprintf.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
 
#define BUF_LEN 128
#define LINE_LEN 1024
static char buf[BUF_LEN];
static char line[LINE_LEN];
static int lp = 0;

int match_line(const char* re, const char* ln)
{
    int nr_re = strlen(re), len = strlen(ln);
    for (int i = 0; i < len - nr_re; i++) {
        if (strncmp(re, ln+i, nr_re) == 0) return 1;
    }
    return 0;
}

//grep re [file]
int main(int argc, char* argv[])
{
    const char* filepath = NULL, *re = NULL;
    if (argc > 1) {
        re = argv[1];
        if (argc > 2) filepath = argv[2];
    }

    int fd = STDIN_FILENO;
    if (filepath) fd = open(filepath, O_RDONLY, 0);
    if (fd >= 0) {
        int len = 0;
        while ((len = read(fd, buf, sizeof buf - 1)) > 0) {
            buf[len] = 0;
            int i = 0; 
            while (i < len) {
                line[lp++] = buf[i++];
                if (buf[i] == '\n') {
                    i++;
                    line[lp++] = '\n';
                    line[lp] = 0;
                    if (match_line(re, line)) 
                        write(STDOUT_FILENO, line, lp);
                    lp = 0;
                }
            }
        }

        if (lp) {
            line[lp] = 0;
            if (match_line(re, line)) 
                write(STDOUT_FILENO, line, lp);
        }
        if (fd != STDIN_FILENO) close(fd);
    }
    return 0;
}



