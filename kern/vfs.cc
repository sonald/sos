#include "vfs.h"
#include "string.h"
#include "common.h"
#include "task.h"
#include "errno.h"
#include "spinlock.h"
#include "disk.h"
#include "ramfs.h"

File cached_files[MAX_NR_FILE];
dev_t rootdev = DEVNO(IDE_MAJOR, 1); //default hda1

VFSManager vfs;
Spinlock vfslock("vfs");

static File* get_free_file()
{
    File* fp = &cached_files[0];
    int i;
    for (i = 0; i < MAX_NR_FILE; i++, fp++) {
        if (fp->ref() == 0) 
            break;
    }

    if (i >= MAX_NR_FILE) return NULL;
    return fp;
}

int sys_mount(const char *src, const char *target, 
        const char *fstype, unsigned long mountflags, const void *data)
{
    //FIXME: TODO: copy user space string into kernel
    return vfs.mount(src, target, fstype, mountflags, data);
}

int sys_unmount(const char *target)
{
    //FIXME: TODO: copy user space string into kernel
    return vfs.unmount(target);
}

// no real mountpoints now, hardcoded for testing
int sys_open(const char *path, int flags, int mode)
{
    (void)flags;
    (void)mode;

    File* f = nullptr;
    inode_t* ip = nullptr;
    int fd = 0;
    auto eflags = vfslock.lock();
    for (fd = 0; fd < FILES_PER_PROC; fd++) {
        if (!current->files[fd])
            break;
    }

    if (fd >= FILES_PER_PROC) {
        fd = -EINVAL;
        goto _out;
    }

    ip = vfs.namei(path);
    if (!ip) {
        fd = -ENOENT;
        goto _out;
    }
    
    f = get_free_file();
    if (!f) {
        //free ip
        fd = -ENOMEM;
        goto _out;
    }

    f->set_inode(ip);
    current->files[fd] = f;

_out:
    vfslock.release(eflags);
    return fd;
}

int sys_close(int fd)
{
    (void)fd;
    return 0;
}

int sys_mmap(struct file *, struct vm_area_struct *)
{
}

int sys_write(int fd, const void *buf, size_t nbyte)
{
    size_t nwrite = 0;
    if (fd == 0) {
        char* p = (char*)buf;
        while (*p && nwrite < nbyte) {
            auto eflags = vfslock.lock();
            kputchar(*p++);
            vfslock.release(eflags);
            ++nwrite;
        }
    }
    return nwrite;
}

int sys_read(int fd, void *buf, size_t nbyte)
{
    (void)fd;
    (void)buf;
    (void)nbyte;
    return 0;
}

// count should be 1 now
int sys_readdir(unsigned int fd, struct dirent *dirp,
        unsigned int count)
{
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

// rootdev should be dev no for like /dev/hda1
void VFSManager::init_root(dev_t rootdev)
{
    return;
    //DevFs* devfs = new Devfs;
    Disk* hd = new Disk;
    hd->init(DEVNO(MAJOR(rootdev), 0));
    auto* part = hd->part(MINOR(rootdev));
    if (part->part_type == PartType::Fat32L) {
        auto* fs = find_fs("fat32");
        kassert(fs);
        _rootfs = fs->spawn((void*)rootdev);        
    }
    delete hd;
    
    if (_rootfs) {
        // debug list root dir
    }
}

int VFSManager::mount(const char *src, const char *target,
        const char *fstype, unsigned long flags, const void *data)
{
    auto* fs = find_fs(fstype);
    if (!fs) {
        return -ENOSYS;
    }
    //TODO: check if mounted
    if (get_mount(target)) {
        kprintf("override mount to %s", target);
    }

    auto* mnt = new mount_info_t;
    auto len = strlen(target);
    mnt->mnt_point = new char[len+1];
    strncpy(mnt->mnt_point, target, len);
    mnt->next = _mounts;
    _mounts = mnt;

    if (strcmp(src, "dev") == 0) {
    } else if (strcmp(fstype, "ramfs") == 0) {
        mnt->fs = fs->spawn((void*)&ramfs_mod_info);
        kassert(get_mount(target) != NULL);

    } else if (strncmp(src, "/dev/hda", 8) == 0) {
        //TODO: extract part num and mount
    }
    return 0;
}

mount_info_t* VFSManager::get_mount(const char* target)
{
    auto* p = _mounts;
    while (p) {
        if (strcmp(p->mnt_point, target) == 0) {
            return p;
        }
        p = p->next;
    }

    return nullptr;
}

int VFSManager::unmount(const char *target)
{
    return 0;
}

file_system_type_t* VFSManager::find_fs(const char* fsname)
{
    auto* p = _fs_types;
    while (p) {
        if (strcmp(fsname, p->fsname) == 0) {
            return p;
        }
        p = p->next;
    }
    return nullptr;
}

void VFSManager::register_fs(const char* fsname, CreateFsFunction func)
{
    file_system_type_t* fst = new file_system_type_t;
    fst->fsname = fsname;
    fst->spawn = func;
    fst->next = _fs_types;
    _fs_types = fst;
}

//TODO: hierachy searching
inode_t* VFSManager::namei(const char* path)
{
    auto* mnt = get_mount("/");
    return mnt->fs->dir_lookup(mnt->fs->root(), path);
}

int VFSManager::read(inode_t* ip, void* buf, size_t nbyte, u32 offset)
{
    return ip->fs->read(ip, buf, nbyte, offset);
}

int VFSManager::write(inode_t* ip, void* buf, size_t nbyte, u32 offset)
{
    return ip->fs->write(ip, buf, nbyte, offset);
}

inode_t* VFSManager::dir_lookup(inode_t* ip, const char* name)
{
    return ip->fs->dir_lookup(ip, name);
}

dentry_t* VFSManager::dir_read(inode_t* ip, int id)
{
    return ip->fs->dir_read(ip, id);
}

