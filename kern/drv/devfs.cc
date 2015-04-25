#include <devfs.h>
#include <common.h>
#include <devices.h>
#include <string.h>
#include <spinlock.h>

Spinlock devfslock {"devfs"};

struct known_dev_ {
    const char* name;
    FsNodeType type;
    dev_t dev;
} known_devs[] = {
    {"tty1", FsNodeType::CharDev, DEVNO(TTY_MAJOR, 1)},
    {"tty2", FsNodeType::CharDev, DEVNO(TTY_MAJOR, 2)},
    {"hda", FsNodeType::BlockDev, DEVNO(IDE_MAJOR, 0)},
    {"zero", FsNodeType::CharDev, DEVNO(ZERO_MAJOR, 0)},
    {"null", FsNodeType::CharDev, DEVNO(NULL_MAJOR, 0)},
};

void DevFs::init()
{
    _iroot = vfs.alloc_inode();
    _iroot->dev = 0;
    _iroot->type = FsNodeType::Dir;
    _iroot->ino = 1;
    _iroot->size = 0;
    _iroot->blksize = 1;
    _iroot->blocks = _iroot->size;
    _iroot->fs = this;
}

dentry_t * DevFs::lookup(inode_t * dir, dentry_t *de)
{
    if (dir->type != FsNodeType::Dir && dir->ino != 1) return NULL;

    size_t i = 0;
    for (i = 0; i < ARRAYLEN(known_devs); i++) {
        if (strcmp(de->name, known_devs[i].name) == 0) {
            break;
        }
    }
    if (i >= ARRAYLEN(known_devs)) return NULL;
    de->ip = vfs.alloc_inode();
    de->ip->ino = i+2;
    de->ip->dev = known_devs[i].dev;
    de->ip->type = known_devs[i].type;
    de->ip->size = 0;
    de->ip->blksize = 1;
    de->ip->blocks = de->ip->size;
    de->ip->fs = this;

    return de;
}

// blocking
ssize_t DevFs::read(File *filp, char * buf, size_t count, off_t * offset)
{
    inode_t* ip = filp->inode();
    if (ip->type != FsNodeType::CharDev && ip->type != FsNodeType::BlockDev) return -EBADF;
    off_t off = filp->off();

    if (ip->type == FsNodeType::CharDev) {
        auto* rdev = chr_device_get(ip->dev);
        if (!rdev) return -ENOENT;
        auto* sp = buf;
        while (count > 0 && (*sp = rdev->read()) >= 0) {
            count--;
            kputchar(*sp); // echoing
            if (*sp == '\n') break;
            sp++;
        }
        *sp = 0;

        count = sp - buf;
    } else if (ip->type == FsNodeType::BlockDev) {
        panic("not implemented");
    }

    filp->set_off(off + count);
    if (offset) *offset = filp->off();
    return count;
}

ssize_t DevFs::write(File *filp, const char * buf, size_t count, off_t *offset)
{
    // write to terminal
    inode_t* ip = filp->inode();
    if (ip->type != FsNodeType::CharDev && ip->type != FsNodeType::BlockDev) return -EBADF;
    (void)offset;

    auto* sp = buf;
    if (ip->type == FsNodeType::CharDev) {
        auto* rdev = chr_device_get(ip->dev);
        if (!rdev) return -ENOENT;

        while (count > 0 && *sp) {
            rdev->write(*sp);
            sp++;
            count--;
        }
    }

    return sp - buf;
}

FileSystem* create_devfs(const void* data)
{
    (void)data;
    auto* fs = new DevFs;
    fs->init();
    return fs;
}
