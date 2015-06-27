#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <printf.h>
 
#define BUF_LEN 128
#define LINE_LEN 1024
static int lp = 0;

static int match_here(const char* re, const char* s);

static int match_star(const char* re, const char* s)
{
    int c = re[0];
    do {
        if (match_here(re+2, s)) return 1;
    } while (*s && (c == *s++ || c == '.'));
    return 0;
}

static int match_here(const char* re, const char* s)
{
    if (!*re) return 1;
    else if (!*s) return 0;

    if (re[1] == '*') {
        return match_star(re, s);
    } else if (*re == '$' && re[1] == 0) {
        return *s == 0;
    } else {
        if (*re == '.' || *s == *re) {
            return match_here(re+1, s+1);
        }
    }

    return 0;
}

// re is regex: only . and * supported
static int match(const char* re, const char* s)
{
    if (*re == '^') return match_here(re+1, s);

    while (*s) {
        if (match_here(re, s)) return 1;
        s++;
    }
    return 0;
}

//grep re [file]
int main(int argc, char* argv[])
{
    char* line = (char*)malloc(LINE_LEN);
    char* buf = (char*)malloc(BUF_LEN);

    int no = 1;

    const char* filepath = NULL, *re = NULL;
    if (argc > 1) {
        re = argv[1];
        if (argc > 2) filepath = argv[2];
    } else {
        printf("usage: grep re file ...\n");
    }

    int fd = STDIN_FILENO;
    if (filepath) fd = open(filepath, O_RDONLY, 0);
    if (fd >= 0) {
        int len = 0;
        while ((len = read(fd, buf, sizeof buf - 1)) > 0) {
            buf[len] = 0;
            int i = 0; 
            while (i < len) {
                line[lp++] = buf[i];
                if (buf[i] == '\n') {
                    line[lp] = 0;
                    if (match(re, line)) {
                        printf("%d: %s", no, line);
                    }
                    no++;
                    lp = 0;
                }
                i++;
            }
        }

        if (lp) {
            line[lp] = 0;
            if (match(re, line)) 
                write(STDOUT_FILENO, line, lp);
        }
        if (fd != STDIN_FILENO) close(fd);
    }

    free(line);
    free(buf);
    return 0;
}




