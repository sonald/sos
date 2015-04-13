#include <devfs.h>
#include <string.h>

DevFs::DevFs()
{
    init();
}

void DevFs::init()
{
    _rootdisk = new Disk;
    auto ma = MAJOR(rootdev);
    _rootdisk->init(DEVNO(ma, 0));
    for (int i = 0; i < _rootdisk->num(); i++) {
        const auto* part = _rootdisk->part(i);
        if (part->part_type == PartType::Fat32L) {
            //vfs.create_fs("ext2");

        } else if (part->part_type == PartType::Linux) {
        }
    }
}


