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
    write(STDOUT_FILENO, buf, strlen(buf));
}

int main(int argc, char* argv[])
{
    myprint("echo\n");
    return 0;
}


