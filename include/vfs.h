#ifndef _VFS_H
#define _VFS_H 

#include "types.h"

// inode flags
#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08 

class FileSystem;
typedef struct inode_s {
    u32 dev;
    u32 length;
    u32 flags;
    u32 gid;
    u32 uid;
    u32 perm;
    FileSystem* fs;
} inode_t;


class FileSystem {
    public:
        virtual u32 read(inode_t* node, u32 offset, u32 size, u8* buffer) = 0;
        virtual u32 write(inode_t* node, u32 offset, u32 size, u8* buffer) = 0;
        virtual void open(inode_t* node, u8 read, u8 write) = 0;
        virtual void close(inode_t* node) = 0;

        virtual struct dirent *readdir(inode_t* node, u32 index) = 0;
        virtual inode_t* finddir(inode_t* node, char* name) = 0;
    protected:
        inode_t * fs_root; // The root of the filesystem.
};

class Initramfs: public FileSystem {
    public:
        virtual u32 read(inode_t* node, u32 offset, u32 size, u8* buffer);
        virtual u32 write(inode_t* node, u32 offset, u32 size, u8* buffer);
        virtual void open(inode_t* node, u8 read, u8 write);
        virtual void close(inode_t* node);

        virtual struct dirent *readdir(inode_t* node, u32 index);
        virtual inode_t* finddir(inode_t* node, char* name);
};

#endif
