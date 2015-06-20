#include <ppm.h>
#include <vfs.h>
#include <sys.h>
#include <common.h>
#include <string.h>
#include <ctype.h>
#include <vm.h>

static int atoi(const char* s)
{
    int i = 0;
    while (*s && isdigit(*s)) {
        i = (i * 10) + (*s - '0');
        s++;
    }
    return i;
}

ppm_t* ppm_load(const char* path)
{
    inode_t* ip = vfs.namei(path);
    if (!ip) return NULL;

    char* buf = new char[ip->size];
    int fd = sys_open(path, O_RDONLY, 0);
    if (fd < 0) {
        kprintf("open logo failed\n");
        delete buf;
        return NULL;
    }

    int len = sys_read(fd, buf, ip->size);
    sys_close(fd);

    if (len < 0) {
        kprintf("load %s failed\n", path);
        delete buf;
        return NULL;
    }

    ppm_t* ppm = (ppm_t*) vmm.kmalloc(len, 1);
    ppm->magic[0] = buf[0];
    ppm->magic[1] = buf[1];
    if (ppm->magic[0] != 'P' && ppm->magic[1] != '6') {
        kprintf("wrong magic\n");
        delete buf;
        delete ppm;
        return NULL;
    }

    char* p = buf+3;
    ppm->width = atoi(p);

    while (isdigit(*p)) p++;
    ppm->height = atoi(++p);

    while (isdigit(*p)) p++;
    ppm->max_color = atoi(++p);
    while (*p != '\n') p++;
    p++;

    len = buf + len - p;
    memcpy(ppm->data, p, len); 
    delete buf;

    return ppm;
}

