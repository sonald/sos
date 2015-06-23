#include <unistd.h>
#include <sprintf.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

int main(int argc, char* const argv[])
{
    int f_long = 0, f_single = 0, f_stream = 0;

    const char* dir = "/";
    if (argc > 1) {
        argc--;
        argv++;

        for (int i = 0; i < argc; i++) {
            auto& av = argv[i];
            if (av[0] && av[0] == '-') {
                switch(av[1]) {
                    case 'l': f_long = 1; break;
                    case '1': f_single = 1; break;
                }
            } else dir = av;
        }
    }

    int fd = open(dir, O_RDONLY, 0);
    if (fd >= 0) {
        struct dirent dire;
        while (readdir(fd, &dire, 1) > 0) {
            char buf[64] = "";
            if (f_stream) 
                snprintf(buf, sizeof buf - 1, "%s, ", dire.d_name);
            else if (f_long) 
                snprintf(buf, sizeof buf - 1, "-rw-r--r-- %s\n", dire.d_name);
            else if (f_single) 
                snprintf(buf, sizeof buf - 1, "%s\n", dire.d_name);
            else
                snprintf(buf, sizeof buf - 1, "%s\t", dire.d_name);
            write(STDOUT_FILENO, buf, strlen(buf));
        }
        write(STDOUT_FILENO, "\n", 1);
        close(fd);
    }
    return 0;
}

