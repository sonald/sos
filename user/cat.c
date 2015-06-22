#include <unistd.h>
#include <sprintf.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
 
int main(int argc, char* argv[])
{
    const char* filepath = NULL; //TODO: from argv[1]

    int fd = STDIN_FILENO;
    if (filepath) fd = open(filepath, O_RDONLY, 0);
    if (fd >= 0) {
        char buf[64];
        int len = 0;
        while ((len = read(fd, buf, sizeof buf - 1)) > 0) {
            buf[len] = 0;
            write(STDOUT_FILENO, buf, len);
        }

        if (fd != STDIN_FILENO) close(fd);
    }
    return 0;
}


