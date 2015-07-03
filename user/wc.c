#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

struct flags_ {
    int word: 1;
    int line: 1;
    int chr: 1;
} flgs = {0, 0, 0};

void wc(int fd, const char* name)
{
    int w = 0, c = 0, l = 0;
    char* buf = (char*)malloc(128);
    int len = 0, inword = 0;

    while ((len = read(fd, buf, 128)) > 0) {
        c += len;
        for (int i = 0; i < len; i++) {
            if (isspace(buf[i])) {
                if (buf[i] == '\n') l++;
                inword = 0;
            } else if (!inword) {
                inword = 1;
                w++;
            }
        }
    }

    free(buf);
    if (flgs.line) printf("%d\t", l);
    if (flgs.word) printf("%d\t", w);
    if (flgs.chr) printf("%d\t", c);
    printf("%s\n", name);
}

int main(int argc, char *argv[])
{
    int nofile = 1;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            char* av = argv[i];
            switch(av[1]) {
                case 'l': flgs.line = 1; break;
                case 'w': flgs.word = 1; break;
                case 'c': flgs.chr = 1; break;
            }
        } else {
            nofile = 0;
            if (!flgs.line && !flgs.word && !flgs.chr) {
                flgs = {1, 1, 1};
            }
            int fd = open(argv[i], O_RDONLY, 0);
            if (fd > 0) {
                wc(fd, argv[i]);
                close(fd);
            }
        }
    }
    
    if (nofile) {
        if (!flgs.line && !flgs.word && !flgs.chr) {
            flgs = {1, 1, 1};
        }
        wc(0, "");
    }
    return 0;
}
