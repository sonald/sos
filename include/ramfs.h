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
        dentry_t * lookup(inode_t * dir, dentry_t *) override;

        ssize_t read(File *, char * buf, size_t count, off_t * offset) override;
        ssize_t write(File *, const char * buf, size_t, off_t *offset) override;
        int readdir(File *, dentry_t *, filldir_t) override;

    private:
        inode_t* _nodes;
        int _nr_nodes;
        initrd_header_t* _sb;
        char* _cmdline;

        void read_inode(inode_t *);
};

FileSystem* create_ramfs(const void*);
#endif
