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
        virtual bool load() = 0;
        virtual u32 read(inode_t* node, u32 offset, u32 size, u8* buffer) = 0;
        virtual u32 write(inode_t* node, u32 offset, u32 size, u8* buffer) = 0;
        virtual void open(inode_t* node, u8 read, u8 write) = 0;
        virtual void close(inode_t* node) = 0;

        virtual struct dirent *readdir(inode_t* node, u32 index) = 0;
        virtual inode_t* finddir(inode_t* node, char* name) = 0;
    protected:
        inode_t * fs_root; // The root of the filesystem.
};


#define NAMELEN 64
#define MAXFILES 64

typedef struct initrd_entry_header
{
   unsigned char magic; // The magic number is there to check for consistency.
   char name[NAMELEN];
   unsigned int offset; // Offset in the initrd the file starts.
   unsigned int length; // Length of the file.
} initrd_entry_header_t;

typedef struct initrd_header
{
    u32 nfiles; 
    initrd_entry_header_t files[MAXFILES];
} initrd_header_t;

class Initramfs: public FileSystem {
    public:
        virtual bool load();
        virtual u32 read(inode_t* node, u32 offset, u32 size, u8* buffer);
        virtual u32 write(inode_t* node, u32 offset, u32 size, u8* buffer);
        virtual void open(inode_t* node, u8 read, u8 write);
        virtual void close(inode_t* node);

        virtual struct dirent *readdir(inode_t* node, u32 index);
        virtual inode_t* finddir(inode_t* node, char* name);
};

#endif
