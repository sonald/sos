#include <fat32.h>
#include <disk.h>
#include <devices.h>

void Fat32Fs::init(dev_t dev)
{
    Disk* hd = new Disk;
    
    this->_dev = dev;
    int partid = MINOR(dev);
    dev_t hdid = DEVNO(MAJOR(dev), 0);
    hd->init(hdid);
    auto* part = hd->part(partid);
    kassert(part->part_type == PartType::Fat32L);

    Buffer* bufp = bio.read(hdid, part->lba_start);
    delete hd;

    fat_bs_t* bs = (fat_bs_t*)&bufp->data;
    kprintf("fat32: %s\n", bs->oem_name);
    bio.release(bufp);
}

int Fat32Fs::read(inode_t* ip, void* buf, size_t nbyte, u32 offset)
{
}

int Fat32Fs::write(inode_t* ip, void* buf, size_t nbyte, u32 offset)
{
}

inode_t* Fat32Fs::dir_lookup(inode_t* ip, const char* name)
{
}

dentry_t* Fat32Fs::dir_read(inode_t* ip, int id)
{
}

FileSystem* create_fat32fs(void* data)
{
    dev_t dev = (dev_t)data;
    auto* fs = new Fat32Fs;
    fs->init(dev);
    return fs;
}

