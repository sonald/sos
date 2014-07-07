#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define NAMELEN 64
#define MAXFILES 64

struct initrd_entry_header 
{
   unsigned char magic; // The magic number is there to check for consistency.
   char name[NAMELEN];
   int32_t offset; // Offset in the initrd the file starts.
   int32_t length; // Length of the file.
}__attribute__((packed));

int main(int argc, char **argv)
{
    int nheaders = (argc-1);
    struct initrd_entry_header headers[MAXFILES];
    memset(headers, 0, sizeof(headers));
    if (nheaders > MAXFILES) nheaders = MAXFILES;

    printf("size of header: %lu\n", sizeof(struct initrd_entry_header));
    unsigned int off = sizeof(struct initrd_entry_header) * MAXFILES + sizeof(int);
    int i;
    for(i = 1; i <= nheaders; i++) {
        printf("writing file %s at 0x%x\n", argv[i], off);
        strncpy(headers[i-1].name, argv[i], NAMELEN-1);
        headers[i-1].offset = off;
        FILE *stream = fopen(argv[i], "r");
        if(stream == 0) {
            printf("Error: file not found: %s\n", argv[i]);
            return 1;
        }

        fseek(stream, 0, SEEK_END);
        headers[i-1].length = ftell(stream);
        off += headers[i-1].length;
        fclose(stream);
        headers[i-1].magic = 0xBF;
    }

    FILE *wstream = fopen("./initramfs.img", "w");
    unsigned char *data = (unsigned char *)malloc(off);
    fwrite(&nheaders, sizeof(int), 1, wstream);
    fwrite(headers, sizeof(struct initrd_entry_header), MAXFILES, wstream);

    for(i = 0; i < nheaders; i++) {
        FILE *stream = fopen(argv[i+1], "r");
        unsigned char *buf = (unsigned char *)malloc(headers[i].length);
        fread(buf, 1, headers[i].length, stream);
        fwrite(buf, 1, headers[i].length, wstream);
        fclose(stream);
        free(buf);
    }

    fclose(wstream);
    free(data);

    return 0;
}
