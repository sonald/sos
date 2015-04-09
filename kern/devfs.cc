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

int DevFs::read(inode_t* ip, void* buf, size_t nbyte, u32 offset)
{
}

int DevFs::write(inode_t* ip, void* buf, size_t nbyte, u32 offset)
{
}

inode_t* DevFs::dir_lookup(inode_t* ip, const char* name)
{
    if (strcmp(name, "hda") == 0) {
    } else if (strcmp(name, "hda") == 0) {
    }
}

dentry_t* DevFs::dir_read(inode_t* ip, int id)
{
}

