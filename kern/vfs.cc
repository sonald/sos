#include "vfs.h"
#include "string.h"
#include "common.h"
#include "task.h"
#include "errno.h"
#include "spinlock.h"

File cached_files[NR_FILE];
inode_t* rooti = NULL;
FileSystem* devices[NDEV] = {NULL, };
Spinlock vfslock("vfs");

static File* get_free_file()
{
    File* fp = &cached_files[0];
    int i;
    for (i = 0; i < NR_FILE; i++, fp++) {
        if (fp->ref() == 0) 
            break;
    }

    if (i >= NR_FILE) return NULL;
    return fp;
}

// return inode from dev driver layer and i number
static inode_t* iget(u32 dev, int ino)
{
    FileSystem* fs = devices[MAJOR(dev)];
    if (!fs) {
        return NULL;
    }

}

static inode_t* namei(const char* path)
{
    //FIXME: hardcoded for ramfs

    FileSystem* fs = devices[RAMFS_MAJOR];
    if (!fs) {
        return NULL;
    }

    return NULL;
}

// no real mountpoints now, hardcoded for testing
int sys_open(const char *path, int flags, int mode)
{
    (void)flags;
    (void)mode;

    int fd = 0;
    vfslock.lock();
    for (fd = 0; fd < FILES_PER_PROC; fd++) {
        if (!current->files[fd])
            break;
    }

    if (fd >= FILES_PER_PROC) {
        vfslock.release();
        return -EINVAL;
    }

    inode_t* ip = namei(path);
    if (!ip) {
        vfslock.release();
        return -ENOENT;
    }
    
    File* f = get_free_file();
    if (!f) {
        vfslock.release();
        return -EINVAL;
    }

    f->set_inode(ip);
    current->files[fd] = f;
    vfslock.release();
    return fd;
}

int sys_close(int fildes)
{
    (void)fildes;
    return 0;
}

int sys_write(int fildes, const void *buf, size_t nbyte)
{
    size_t nwrite = 0;
    if (fildes == 0) {
        char* p = (char*)buf;
        while (*p && nwrite < nbyte) {
            vfslock.lock();
            kputchar(*p++);
            vfslock.release();
            ++nwrite;
        }
    }
    return nwrite;
}

int sys_read(int fildes, void *buf, size_t nbyte)
{
    (void)fildes;
    (void)buf;
    (void)nbyte;
    return 0;
}


int File::read(void* buf, size_t nbyte)
{
    (void)buf;
    (void)nbyte;
    return -EBADF;
}

int File::write(void* buf, size_t nbyte)
{
    (void)buf;
    (void)nbyte;
    return -EBADF;
}

int File::open(int flags, int mode)
{
    (void)flags;
    (void)mode;
    return -EBADF;
}

int File::close()
{
    return -EBADF;
}

