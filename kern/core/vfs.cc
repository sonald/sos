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
    auto oflags = vfslock.lock();
    for (i = 0; i < MAX_NR_FILE; i++, fp++) {
        if (fp->ref() == 0) {
            memset(fp, 0, sizeof *fp);
            vfslock.release(oflags);
            return fp;
        }
    }
    vfslock.release(oflags);
    return NULL;
}

static int fdalloc()
{
    int fd = -EINVAL;
    auto oflags = vfslock.lock();
    for (fd = 0; fd < FILES_PER_PROC; fd++) {
        if (!current->files[fd]) {
            vfslock.release(oflags);
            return fd;
        }
    }
    vfslock.release(oflags);
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
    (void)mode;

    File* f = nullptr;
    inode_t* ip = nullptr;
    int fd = -1;
    auto eflags = vfslock.lock();
    if ((fd = fdalloc()) < 0) {
        fd = -ENOMEM;
        goto _out;
    }
    // kprintf("%s:fd %d\n", __func__, fd);
    
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
    if (!(flags & O_WRONLY)) f->set_readable();
    if ((flags & O_WRONLY) || (flags & O_RDWR)) f->set_writable();

    current->files[fd] = f;

_out:
    vfslock.release(eflags);
    return fd;
}

int sys_close(int fd)
{
     //kprintf("%s:fd %d\n", __func__, fd);
    auto* filp = current->files[fd];
    kassert(filp && filp->ref() > 0);

    auto eflags = vfslock.lock();
    filp->put();
    if (filp->ref() == 0) {
        memset(filp, 0, sizeof *filp);
    }
    current->files[fd] = NULL;
    vfslock.release(eflags);
    return 0;
}

int sys_dup2(int fd, int fd2)
{
    if (fd == fd2) return 0;

    auto* filp = current->files[fd2];
    if (filp) sys_close(fd2); 

    auto eflags = vfslock.lock();
    current->files[fd]->dup();
    current->files[fd2] = current->files[fd];
    vfslock.release(eflags);
    return 0;
}

int sys_dup(int fd)
{
    int newfd = -1;

    auto eflags = vfslock.lock();
    if ((newfd = fdalloc()) < 0) {
        newfd = -ENOMEM;
        goto _out;
    }
    
    current->files[fd]->dup();
    current->files[newfd] = current->files[fd];

_out:
    vfslock.release(eflags);
    return newfd;
}

int sys_pipe(int fd[2])
{
    File* f[2] = {NULL, NULL};
    Pipe* p = new Pipe;
    int ret = 0;

    auto eflags = vfslock.lock();
    for (int i = 0; i < 2; i++) {
        if (!(f[i] = get_free_file())) {
            ret = -ENFILE;
            goto _bad;
        }
        if (i == 0) p->readf = f[i];
        else p->writef = f[i];

        if ((fd[i] = fdalloc()) < 0) {
            ret = -EMFILE;
            goto _bad;
        }
        current->files[fd[i]] = f[i];
        f[i]->set_pipe(p);
        if (i == 0) f[i]->set_readable();
        else f[i]->set_writable();
    }
    kassert(f[0] != f[1]);

    vfslock.release(eflags);
    return ret;

_bad:
    if (f[0] && f[0]->ref()) f[0]->put();
    if (f[1] && f[1]->ref()) f[1]->put();
    if (p) delete p;

    vfslock.release(eflags);
    return ret;
}

int sys_mmap(struct file *, struct vm_area_struct *)
{
    return 0;
}

int sys_write(int fd, const void *buf, size_t nbyte)
{
    if (fd >= MAX_NR_FILE) return -EINVAL; 

    auto* filp = current->files[fd];
    if (!filp->writable()) return -EPERM;

    if (filp->type() == File::Type::Pipe) {
        return filp->pipe()->write(buf, nbyte);
    }

    inode_t* ip = filp->inode();
    if (ip->type != FsNodeType::CharDev) {
        kprintf("%s: fd %d, not chardev, don not support\n", __func__, fd);
        return -1;
    }

    auto eflags = vfslock.lock();
    off_t off = filp->off();
    auto ret = ip->fs->write(filp, (char*)buf, nbyte, &off);
    vfslock.release(eflags);
    return ret;
}

int sys_read(int fd, void *buf, size_t nbyte)
{
    if (fd >= MAX_NR_FILE) return -EINVAL; 

    auto* filp = current->files[fd];
    if (!filp->readable()) return -EPERM;

    if (filp->type() == File::Type::Pipe) {
        return filp->pipe()->read(buf, nbyte);
    }

    inode_t* ip = filp->inode();
    auto eflags = vfslock.lock();
    off_t off = filp->off();
    auto ret = ip->fs->read(filp, (char*)buf, nbyte, &off);
    vfslock.release(eflags);
    return ret;
}

// ignore count, should only be 1 by now
// ret > 0 success, = 0 end of dir, < 0 error
int sys_readdir(unsigned int fd, struct dirent *dirp, unsigned int count)
{
    // kprintf("%s:fd %d\n", __func__, fd);
    // dir should be opened
    auto* filp = current->files[fd];
    inode_t* ip = filp->inode();
    (void)count;

    kassert(ip->type == FsNodeType::Dir);

    auto* fs = ip->fs;
    auto* de = vfs.alloc_entry();
    off_t off = filp->off();
    int ret = fs->readdir(filp, de, 0);
    if (ret > 0) {
        dirp->d_ino = de->ip->ino;
        dirp->d_off = off;
        dirp->d_reclen = sizeof *dirp;
        strncpy(dirp->d_name, de->name, NAMELEN);
    }
    vfs.dealloc_entry(de);
    return ret;
}



int Pipe::write(const void *buf, size_t nbyte)
{
    const char* p = (const char*)buf;
    int nr_written = nbyte;

    while (nr_written > 0) {
        if (pbuf->full()) {
            if (readf == NULL) {
                //TODO: send signal PIPE
                break;
            }
            wakeup(this);
            auto oflags = vfslock.lock();
            sleep(&vfslock, this);
            vfslock.release(oflags);
        } else {
            nr_written--;
            pbuf->write(*p++);
        }
    }

    wakeup(this);
    return nbyte - nr_written;
}

int Pipe::read(void *buf, size_t nbyte)
{
    char* p = (char*)buf;
    size_t nr_read = 0;

    while (nr_read < nbyte) {
        if (pbuf->empty()) {
            if (writef == NULL) {
                break; // write end is closed
            }
            wakeup(this);
            auto oflags = vfslock.lock();
            sleep(&vfslock, this);
            vfslock.release(oflags);
        } else {
            nr_read++;
            *p++ = pbuf->read();
        }
    }
    return nr_read;
}

File::File(Type ty)
    : _ip(NULL), _off(0), _ref(0), _type(ty), _pipe(NULL) 
{
}

void File::set_pipe(Pipe* p)
{
    kassert(_type == Type::None);
    _type = Type::Pipe;
    _ref = 1;
    _pipe = p;
}

void File::set_inode(inode_t* ip) {
    kassert(_type == Type::None);
    _ref = 1;
    _ip = ip;
    _type = Type::Inode;
}

//TODO: update disk if dirty
void File::put() 
{
    if (--_ref > 0) {
        return;
    } else {
        if (_type == Type::Inode) {
            vfs.dealloc_inode(_ip);
            _ip = NULL;
        } else if (_type == Type::Pipe) {
            //FIXME: flush remaining data or just close ?
            if (_pipe->readf == this) {
                _pipe->readf = NULL;
                wakeup(_pipe);
            } else if (_pipe->writef == this) {
                _pipe->writef = NULL;
                wakeup(_pipe);
            }

            if (_pipe->readf == NULL && _pipe->writef == NULL) {
                delete _pipe;
                _pipe = NULL;
            }
        }
        _type = Type::None;
    }
}

File::~File()
{
}

// rootdev should be dev no for like /dev/hda1
void VFSManager::init_root(dev_t rootdev)
{
    //DevFs* devfs = new Devfs;
    Disk* hd = new Disk;
    hd->init(DEVNO(MAJOR(rootdev), 0));
    auto* part = hd->part(MINOR(rootdev)-1);
    if (part->part_type == PartType::Fat32L) {
        // char devname[NAMELEN+1] = "rootfs";
        // snprintf(devname, NAMELEN, "/dev/hd%c%d", 'a', MINOR(rootdev));
        mount("rootfs", "/", "fat32", 0, 0);
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

    if (strcmp(src, "devfs") == 0) {
        mnt->fs = fs->spawn(data);

    } else if (strcmp(fstype, "ramfs") == 0) {
        mnt->fs = fs->spawn(data);
        kassert(get_mount(target) != NULL);

    } else if (strcmp(src, "rootfs") == 0) {
        dev_t devno = DEVNO(IDE_MAJOR, 1);
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
    auto oflags = vfslock.lock();
    while (p) {
        if (strcmp(p->mnt_point, target) == 0) {
            vfslock.release(oflags);
            return p;
        }
        p = p->next;
    }
    vfslock.release(oflags);

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
    auto oflags = vfslock.lock();
    while (p) {
        if (strcmp(fsname, p->fsname) == 0) {
            vfslock.release(oflags);
            return p;
        }
        p = p->next;
    }
    vfslock.release(oflags);

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
    // kprintf("get mount %s, new_path %s ", mnt->mnt_point, stripped);
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

    //TODO: need to impl cwd for proc
    int len = strlen(path)+1;
    char* full_path = new char[len+1];
    if (path && path[0] != '/') {
        snprintf(full_path, len, "/%s", path);
    } else {
        strcpy(full_path, path);
    }

    //TODO: recursively lookup
    char* new_path = nullptr;
    auto* mnt = find_mount(full_path, &new_path);
    //delete full_path;

    if (strcmp(new_path, "/") == 0) {
        vfslock.release(eflags);
        auto* dup = alloc_inode();
        memcpy(dup, mnt->fs->root(), sizeof *dup);
        return dup;
    }

    auto* old = new_path;
    char part[NAMELEN+1];
    auto* ip = mnt->fs->root();
    while ((new_path = path_part(new_path, part)) != NULL) {
        if ((ip = dir_lookup(ip, part)) == NULL) {
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
        de->ip = NULL;
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
        if (de->ip == 0 && de->name[0] == 0)
            break;
    }
    vfslock.release(eflags);

    if (i >= MAX_NR_DENTRY)
        panic("no mem for dentry");
    return de;
}

void VFSManager::dealloc_entry(dentry_t* de)
{
    auto eflags = vfslock.lock();
    if (de->ip) dealloc_inode(de->ip);
    memset(de, 0, sizeof *de);
    vfslock.release(eflags);
}

inode_t* VFSManager::alloc_inode()
{
    inode_t* ip = &cached_inodes[0];
    int i;

    auto eflags = vfslock.lock();

    for (i = 0; i < MAX_NR_INODE; i++, ip++) {
        if (ip->ino == 0 && ip->dev == 0 && ip->blksize == 0) break;
    }
    vfslock.release(eflags);

    if (i >= MAX_NR_INODE)
        panic("no mem for inode");
    return ip;
}

void VFSManager::dealloc_inode(inode_t* ip)
{
    auto eflags = vfslock.lock();
    if (ip->data) delete (char*)ip->data;
    memset(ip, 0, sizeof *ip);
    vfslock.release(eflags);
}
