#include <unistd.h>
#include <printf.h>

#define BUF_SZ 128
char echobuf[128];
 
int main(int argc, char* argv[])
{
    argc--;
    argv++;
    while (argc-- > 0) {
        printf("%s ", *argv++);
    }
    printf("\n");
    return 0;
}


