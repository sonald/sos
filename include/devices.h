#ifndef _DEVICES_H
#define _DEVICES_H 

#include "types.h"
#include "blkio.h"

#define MAJOR(a) (((u32)(a))>>16)
#define MINOR(a) ((a) & 0xFFFF)
#define DEVNO(ma, mi) ((((u32)(ma))<<16) | (((u32)(mi)) & 0xFFFF))

enum DeviceType {
    TTY_MAJOR  = 0,
    RAMFS_MAJOR = 1,
    IDE_MAJOR = 2,
    FLOPPY_MAJOR = 3,
    SCSI_MAJOR = 4,
    NDEV = 5
};

struct BlockDevice 
{
    dev_t dev;
    BlockDevice* prev;
    BlockDevice* next;
    virtual bool read(Buffer* bufp) = 0;        
    virtual bool write(Buffer* bufp) = 0;
};

struct CharDevice
{
    dev_t dev;
    CharDevice* prev;
    CharDevice* next;
    virtual char read(dev_t dev) = 0;
    virtual bool write(dev_t dev, char ch) = 0;
};

extern struct BlockDevice* block_devices;
extern struct CharDevice* char_devices;

void chr_device_register(dev_t dev, CharDevice* chrdev);
void blk_device_register(dev_t dev, BlockDevice* blkdev);

BlockDevice* blk_device_get(dev_t dev);
CharDevice* chr_device_get(dev_t dev);

#endif
