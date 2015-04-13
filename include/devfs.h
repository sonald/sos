#ifndef _SOS_DEVFS_H
#define _SOS_DEVFS_H 

#include <disk.h>
#include <vfs.h>

// should mount at '/dev' now
class DevFs: public FileSystem {
    public:
        DevFs();
        void init();

    private:
        Disk* _rootdisk;
};

#endif

