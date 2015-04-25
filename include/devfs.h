#ifndef _SOS_DEVFS_H
#define _SOS_DEVFS_H

#include <vfs.h>

// should mount at '/dev' now
class DevFs: public FileSystem {
    public:
        DevFs(): FileSystem() {}
        void init();
        dentry_t * lookup(inode_t * dir, dentry_t *) override;

        ssize_t read(File *, char * buf, size_t count, off_t * offset) override;
        ssize_t write(File *, const char * buf, size_t, off_t *offset) override;

    private:
    	int _nodes;
};

FileSystem* create_devfs(const void*);

#endif

