#ifndef _VFS_H
#define _VFS_H 

#include "types.h"
#include "fcntl.h"
#include "devices.h"

#define NAMELEN 64
#define MAX_NR_FILE   64      // max files to open system-wide

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
    u32 dev;  // major and minor
    u32 size;
    FsNodeType type;
    u32 ino;

    FileSystem* fs;
} inode_t;

typedef struct dentry_s {
    char name[NAMELEN];
    u32 ino;
} dentry_t;

class File {
    public:
        enum class Type {
            None, Pipe, Inode
        };

        File(): _ip(NULL), _off(0), _ref(0),_type(Type::None) {}

        int read(void* buf, size_t nbyte);
        int write(void* buf, size_t nbyte);
        int open(int flags, int mode);
        int close();

        int ref() const { return _ref; }
        u32 off() const { return _off; }

        void set_inode(inode_t* ip) { _ip = ip; }

    protected:
        inode_t *_ip;
        u32 _off;
        int _ref;
        Type _type;
};

struct fs_super_s {
    uint8_t mount_count;
};

class FileSystem {
    public:
        friend class VFSManager;

        FileSystem(): _iroot(NULL) {}
        //virtual void read_super();

        virtual int read(inode_t* ip, void* buf, size_t nbyte, u32 offset);
        virtual int write(inode_t* ip, void* buf, size_t nbyte, u32 offset);

        virtual inode_t* dir_lookup(inode_t* ip, const char* name);
        virtual dentry_t* dir_read(inode_t* ip, int id);

        inode_t* root() const { return _iroot; }

    protected:
        inode_t* _iroot;
        fs_super_s* _sb;

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

using CreateFsFunction = FileSystem* (*)(void*);
typedef struct file_system_type_s {
    const char* fsname;
    CreateFsFunction spawn;
    struct file_system_type_s *next;
} file_system_type_t;

class VFSManager {
    public:
        void init_root(dev_t rootdev); //bootstrap root dev
        void register_fs(const char* fsname, CreateFsFunction func);
        file_system_type_t* find_fs(const char* fsname);

        int mount(const char *src, const char *target, const char *fstype,
                unsigned long mountflags, const void *data);
        int unmount(const char *target);
        mount_info_t* get_mount(const char* target);

        inode_t* namei(const char* path);
        int read(inode_t* ip, void* buf, size_t nbyte, u32 offset);
        int write(inode_t* ip, void* buf, size_t nbyte, u32 offset);
        inode_t* dir_lookup(inode_t* ip, const char* name);
        dentry_t* dir_read(inode_t* ip, int id);

    private:
        FileSystem* _fs_list {nullptr};
        file_system_type_t* _fs_types {nullptr};
        FileSystem* _rootfs {nullptr}; // fs for root mountpoint
        mount_info_t* _mounts {nullptr};

};

extern dev_t rootdev;
extern VFSManager vfs;

#endif
