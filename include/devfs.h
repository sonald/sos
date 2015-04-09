#ifndef _SOS_DEVFS_H
#define _SOS_DEVFS_H 

#include <disk.h>
#include <vfs.h>

// should mount at '/dev' now
class DevFs: public FileSystem {
    public:
        DevFs();
        void init();

        int read(inode_t* ip, void* buf, size_t nbyte, u32 offset) override;
        int write(inode_t* ip, void* buf, size_t nbyte, u32 offset) override;
        inode_t* dir_lookup(inode_t* ip, const char* name) override;
        dentry_t* dir_read(inode_t* ip, int id) override;

    private:
        Disk* _rootdisk;
};

#endif

