#ifndef _VFS_H
#define _VFS_H

#include <types.h>
#include <fcntl.h>
#include <devices.h>
#include <sos/limits.h>
#include <errno.h>
#include <ringbuf.h>

class Disk;

enum class FsNodeType: uint8_t {
    Inval,
    Dir,
    File,
    BlockDev,
    CharDev
};

class FileSystem;
// in-memory inode
typedef struct inode_s {
    dev_t dev;  // major and minor
    u32 ino;
    loff_t size;  // size in bytes
    loff_t blocks;
    size_t blksize;
    time_t mtime;
    time_t atime;
    time_t ctime;
    umode_t mode;
    FsNodeType type;
    int ref; // in-memory copies
    void* data; // fs specific
    FileSystem* fs;
} inode_t;

typedef struct dentry_s {
    char name[NAMELEN+1];
    struct dentry_s *parent;
    inode_t *ip;
} dentry_t;

using PipeBuffer = RingBuffer<char, 1024>;
class File;
struct Pipe {
    public:
        File* readf;
        File* writef;
        PipeBuffer pbuf;

        int write(const void *buf, size_t nbyte);
        int read(void *buf, size_t nbyte);
};

class File {
    public:
        enum class Type {
            None, Pipe, Inode
        };
        enum Mode {
            Readable = 0x01,
            Writable = 0x02,
        };


        File(Type ty = Type::None);
        ~File(); 

        void dup() { _ref++; }
        void put();

        Type type() const { return _type; }
        int ref() const { return _ref; }
        off_t off() const { return _off; }
        void set_off(off_t off) { _off = off; }
        inode_t* inode() { return _ip; }
        void set_inode(inode_t* ip);
        void set_pipe(Pipe* p);
        Pipe* pipe() { return _pipe; }

        void set_readable() { _mode |= Readable; }
        void set_writable() { _mode |= Writable; }
        bool readable() const { return _mode & Readable; }
        bool writable() const { return _mode & Writable; }

    protected:
        inode_t *_ip;
        off_t _off;
        int _ref;
        Type _type;
        Pipe *_pipe;
        int _mode;
};

typedef struct fs_super_s {
    uint8_t mount_count;
} fs_super_t;

typedef void* filldir_t;
class FileSystem {
    public:
        friend class VFSManager;

        FileSystem(): _iroot(NULL) {}

        // virtual void dirty_inode(inode_t *) {}
        // virtual void write_inode(inode_t *, int) {}
        // virtual void put_inode(inode_t *) {}
        // virtual void drop_inode(inode_t *) {}
        // virtual void delete_inode(inode_t *) {}

        // virtual int create(inode_t * dir, struct dentry *, int mode) {}
        // virtual int link(struct dentry * old, inode_t * dir, struct dentry *new_de) {}
        // virtual int unlink(inode_t * dir, dentry_t *) {}
        // virtual int mkdir(inode_t *, dentry_t *, int) {}
        // virtual int rmdir(inode_t *, dentry_t *) {}
        // virtual int mknod(inode_t *, dentry_t *, int, dev_t) {}
        // virtual int rename(inode_t *, dentry_t *, inode_t *, dentry_t *) {}
        // virtual int readlink(dentry_t *, char * buf,int len) {}
        // virtual int open(inode_t *, struct file *) {}
        // virtual int release(inode_t *, struct file *) {}

        virtual dentry_t * lookup(inode_t*, dentry_t*) { return NULL; }

        virtual off_t llseek(File *, off_t , int ) { return -EINVAL; }
        virtual ssize_t read(File *, char * , size_t , off_t* ) { return -EINVAL; }
        virtual ssize_t write(File *, const char * , size_t, off_t* ) { return -EINVAL; }
        virtual int readdir(File *, dentry_t *, filldir_t) { return -EINVAL; }

        inode_t* root() const { return _iroot; }

    protected:
        inode_t* _iroot;
        fs_super_t* _sb;

        FileSystem* _prev, *_next;
};

/**
 * NOTE: mount point handling is extremely complex, I have observed
 * linux 1.0 and linux 0.11 and modern linux, the schema evolves
 * a lot! so I'll take a simple wrong way to do it first.
 * mnts ordered in stack
 */
typedef struct mount_info_s {
    char* mnt_point;
    FileSystem* fs;
    struct mount_info_s *next;
} mount_info_t;

using CreateFsFunction = FileSystem* (*)(const void*);
typedef struct file_system_type_s {
    const char* fsname;
    CreateFsFunction spawn;
    struct file_system_type_s *next;
} file_system_type_t;

class VFSManager {
    public:
        void init();
        void init_root(dev_t rootdev); //bootstrap root dev
        void register_fs(const char* fsname, CreateFsFunction func);
        file_system_type_t* find_fs(const char* fsname);

        int mount(const char *src, const char *target, const char *fstype,
                unsigned long mountflags, const void *data);
        int unmount(const char *target);
        mount_info_t* get_mount(const char* target);
        // find mount point for path, and return new_path by stripping mount prefix
        mount_info_t* find_mount(const char* path, char**new_path);

        inode_t* namei(const char* path);
        // wrapper for fs::lookup
        inode_t* dir_lookup(inode_t* ip, const char* name);

        dentry_t* alloc_entry();
        void dealloc_entry(dentry_t*);
        inode_t* alloc_inode(dev_t dev, uint32_t ino);
        void dealloc_inode(inode_t*);

    private:
        FileSystem* _fs_list {nullptr};
        file_system_type_t* _fs_types {nullptr};
        FileSystem* _rootfs {nullptr}; // fs for root mountpoint
        mount_info_t* _mounts {nullptr};
};

extern dev_t rootdev;
extern VFSManager vfs;

#endif
