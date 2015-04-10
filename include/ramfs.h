#ifndef _RAMFS_H
#define _RAMFS_H 

#include "types.h"
#include "vfs.h"

typedef struct initrd_header initrd_header_t;
typedef struct ramfs_mod_info_s {
    char* addr;
    size_t size;
    char* cmdline;
} ramfs_mod_info_t;

class Ramfs: public FileSystem {
    public: 
        Ramfs(): FileSystem() {}
        void init(char* addr, size_t size, const char* cmdline);

        int read(inode_t* ip, void* buf, size_t nbyte, u32 offset) override;
        int write(inode_t* ip, void* buf, size_t nbyte, u32 offset) override;
        inode_t* dir_lookup(inode_t* ip, const char* name) override;
        dentry_t* dir_read(inode_t* ip, int id) override;

    private:
        inode_t* _nodes;
        int _nr_nodes;
        initrd_header_t* _sb;
        char* _cmdline;
};

extern ramfs_mod_info_t ramfs_mod_info;
FileSystem* create_ramfs(void*);
#endif
