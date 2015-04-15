#include "vfs.h"
#include "string.h"
#include "common.h"
#include "task.h"
#include "errno.h"
#include "spinlock.h"
#include "disk.h"
#include "ramfs.h"
#include <dirent.h>
#include <sos/limits.h>
#include <sprintf.h>

File cached_files[MAX_NR_FILE];
inode_t cached_inodes[MAX_NR_INODE];
dentry_t cached_dentries[MAX_NR_DENTRY];

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
    memset(fp, 0, sizeof *fp);
    return fp;
}

static int fdalloc()
{
    int fd = -EINVAL;
    for (fd = 0; fd < FILES_PER_PROC; fd++) {
        if (!current->files[fd])
            break;
    }

    if (fd >= FILES_PER_PROC) {
        fd = -EINVAL;
    }

    return fd;
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

int sys_open(const char *path, int flags, int mode)
{
    (void)flags;
    (void)mode;

    File* f = nullptr;
    inode_t* ip = nullptr;
    int fd = -1;
    auto eflags = vfslock.lock();
    if ((fd = fdalloc()) < 0) {
        fd = -ENOMEM;
        goto _out;
    }

    ip = vfs.namei(path);
    if (!ip) {
        fd = -ENOENT;
        goto _out;
    }
    
    f = get_free_file();
    if (!f) {
        vfs.dealloc_inode(ip);
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
    auto* filp = current->files[fd];
    inode_t* ip = filp->inode();
    kassert(filp && ip);

    auto eflags = vfslock.lock();   
    memset(filp, 0, sizeof *filp);
    current->files[fd] = NULL;
    vfs.dealloc_inode(ip); 

    vfslock.release(eflags);
    return 0;
}

int sys_mmap(struct file *, struct vm_area_struct *)
{
    return 0;
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
    auto* filp = current->files[fd];
    inode_t* ip = filp->inode();
    off_t off = filp->off();
    return ip->fs->read(filp, (char*)buf, nbyte, &off);
}

// ignore count, should only be 1 by now
int sys_readdir(unsigned int fd, struct dirent *dirp, unsigned int count)
{
    // dir should be opened
    auto* filp = current->files[fd];
    inode_t* ip = filp->inode();
    (void)count;

    kassert(ip->type == FsNodeType::Dir);

    auto* fs = ip->fs;
    auto* de = vfs.alloc_entry();
    off_t off = filp->off();
    int ret = fs->readdir(filp, de, 0);
    if (ret == 0) {
        dirp->d_ino = ip->ino;
        dirp->d_off = off;
        dirp->d_reclen = sizeof *dirp;
        strncpy(dirp->d_name, de->name, NAMELEN);
    }
    vfs.dealloc_entry(de);
    return ret;
}

// rootdev should be dev no for like /dev/hda1
void VFSManager::init_root(dev_t rootdev)
{
    //DevFs* devfs = new Devfs;
    Disk* hd = new Disk;
    hd->init(DEVNO(MAJOR(rootdev), 0));
    auto* part = hd->part(MINOR(rootdev)-1);
    if (part->part_type == PartType::Fat32L) {
        char devname[NAMELEN+1];
        sprintf(devname, NAMELEN, "/dev/hd%c%d", 'a', MINOR(rootdev));
        mount(devname, "/", "fat32", 0, 0);
    }
    delete hd;
}

int VFSManager::mount(const char *src, const char *target,
        const char *fstype, unsigned long flags, const void *data)
{
    (void)flags;
    (void)data;

    auto* fs = find_fs(fstype);
    if (!fs) { return -ENOSYS; }

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
        mnt->fs = fs->spawn(data);
        kassert(get_mount(target) != NULL);

    } else if (strncmp(src, "/dev/hda", 8) == 0) {
        dev_t devno = DEVNO(IDE_MAJOR, src[8] - '0');
        mnt->fs = fs->spawn((void*)devno);
    }

    if (target[0] == '/' && target[1] == 0) {
        _rootfs = mnt->fs;
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
    (void)target;
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

//assume path is normalized
static char* parent_path(char* path) 
{
    int n, i;
    for (n = strlen(path), i = n-1; i >= 0; i--) {
        if (i == 0 || path[i] == '/') {
            break;
        }
    }

    if (i != 0) path[i] = 0;
    else path[1] = 0;
    return path;
}

static char* strip_parent(const char* path, const char* prefix)
{
    int n1 = strlen(prefix), n2 = strlen(path);

    if (n1 == n2) {
        auto* new_path = new char[2];
        new_path[0] = '/';
        new_path[1] = 0;
        return new_path;
    }

    auto* new_path = new char[n2-n1+1];    
    memcpy(new_path, path+n1, n2-n1+1);
    return new_path;
}

// Examples:
//   path_part("a/bb/c", name) = "bb/c", setting name = "a"
//   path_part("///a//bb", name) = "bb", setting name = "a"
//   path_part("a", name) = "", setting name = "a"
//   path_part("", name) = skipelem("////", name) = 0
static char* path_part(char* path, char* part)
{
    auto* p = path;
    while (*p == '/') p++;
    if (!*p) return NULL;

    path = p;
    while (*p && *p != '/') p++;
    auto n = min(p - path, NAMELEN);

    memcpy(part, path, n);
    part[n] = '\0';
    return p;
}

mount_info_t* VFSManager::find_mount(const char* path, char**new_path)
{
    char* pth = new char[strlen(path)+1];
    strcpy(pth, path);
    mount_info_t* mnt = nullptr;
    while (1) {
        if ((mnt = get_mount(pth)) != NULL) {
            break;
        }
        pth = parent_path(pth);
    }
    delete pth;

    auto* stripped = strip_parent(path, mnt->mnt_point);
    kprintf("get mount %s, new_path %s ", mnt->mnt_point, stripped);    
    if (new_path) {
        *new_path = stripped;
    } else {
        delete stripped;
    }

    return mnt;
}

inode_t* VFSManager::namei(const char* path)
{
    auto eflags = vfslock.lock();

    //TODO: recursively lookup
    char* new_path = nullptr;
    auto* mnt = find_mount(path, &new_path);

    if (strcmp(new_path, "/") == 0) {
        vfslock.release(eflags);
        return mnt->fs->root();
    }

    auto* old = new_path;
    char part[NAMELEN+1];
    auto* ip = mnt->fs->root();
    while ((new_path = path_part(new_path, part)) != NULL) {
        kprintf("%s: part %s, path %s\n", __func__, part, new_path);
        if ((ip = dir_lookup(ip, part)) == NULL) {
            kprintf("%s failed to find %s\n", __func__, path);            
            break;
        }
    }
    delete old;

    vfslock.release(eflags);
    return ip;
}

inode_t* VFSManager::dir_lookup(inode_t* dir, const char* name)
{
    auto* fs = dir->fs;
    dentry_t* de = alloc_entry();

    auto eflags = vfslock.lock();
    inode_t* ip = NULL;
    strncpy(de->name, name, NAMELEN);
    de->name[NAMELEN] = 0;
    if (fs->lookup(dir, de)) {
        ip = de->ip;
    }

    dealloc_entry(de);
    vfslock.release(eflags);
    return ip;
}

dentry_t* VFSManager::alloc_entry()
{
    dentry_t* de = &cached_dentries[0];
    int i;

    auto eflags = vfslock.lock();    
    for (i = 0; i < MAX_NR_DENTRY; i++, de++) {
        if (de->ip == 0) 
            break;
    }
    vfslock.release(eflags);

    if (i >= MAX_NR_DENTRY) return NULL;
    return de;
}

void VFSManager::dealloc_entry(dentry_t* de)
{
    auto eflags = vfslock.lock();
    memset(de, 0, sizeof *de);    
    vfslock.release(eflags);  
}

inode_t* VFSManager::alloc_inode()
{
    inode_t* ip = &cached_inodes[0];
    int i;

    auto eflags = vfslock.lock();

    for (i = 0; i < MAX_NR_INODE; i++, ip++) {
        if (ip->ino == 0) break;
    }
    vfslock.release(eflags);

    if (i >= MAX_NR_INODE) return NULL;
    return ip;
}

void VFSManager::dealloc_inode(inode_t* ip)
{
    auto eflags = vfslock.lock();
    memset(ip, 0, sizeof *ip);
    vfslock.release(eflags);   
}
