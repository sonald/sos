#ifndef _VFS_H
#define _VFS_H 

#include "types.h"
#include "fcntl.h"

#define NAMELEN 64

#define MAJOR(a) (((u32)(a))>>16)
#define MINOR(a) ((a) & 0xFFFF)
#define DEVNO(ma, mi) ((((u32)(ma))<<16) | (((u32)(mi)) & 0xFFFF))

enum NODE_TYPE {
    I_INVAL,
    I_DIR,
    I_FILE,
    I_DEV
};

// in-memory inode
typedef struct inode_s {
    u32 dev;  // major and minor
    u32 size;
    u8 type;
    u32 ino;
} inode_t;

typedef struct dentry_s {
    char name[NAMELEN];
    u32 ino;
} dentry_t;

class File {
    public:
        enum Type {
            FT_NONE, FT_PIPE, FT_INODE
        };

        File(): _ip(NULL), _off(0), _ref(0),_type(FT_NONE) {}

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

class FileSystem {
    public:
        FileSystem(): _iroot(NULL) {}
        virtual int read(inode_t* ip, void* buf, size_t nbyte, u32 offset);
        virtual int write(inode_t* ip, void* buf, size_t nbyte, u32 offset);

        virtual inode_t* dir_lookup(inode_t* ip, const char* name);
        virtual dentry_t* dir_read(inode_t* ip, int id);

        inode_t* root() const { return _iroot; }
    protected:
        inode_t* _iroot;
};

#define TTY_MAJOR 0
#define RAMFS_MAJOR 1
#define IDE_MAJOR   2
#define FLOPPY_MAJOR  3

#define NDEV  4

#define NR_FILE 64      // max files to open system-wide
extern File cached_files[NR_FILE];
extern inode_t* rooti;

extern FileSystem* devices[NDEV];

#endif
