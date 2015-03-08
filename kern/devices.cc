#include "devices.h"


struct BlockDevice* block_devices = nullptr;
struct CharDevice* char_devices = nullptr;

void chr_device_register(dev_t dev, CharDevice* chrdev)
{
    if (!char_devices) {
        char_devices = chrdev;
        chrdev->next = chrdev;
        chrdev->prev = chrdev;
    } else {
        char_devices->next->prev = chrdev;
        chrdev->prev = char_devices;
        chrdev->next = char_devices->next;
        char_devices->next = chrdev;
    }
}
    
void blk_device_register(dev_t dev, BlockDevice* blkdev)
{
    if (!block_devices) {
        block_devices = blkdev;
        blkdev->next = blkdev;
        blkdev->prev = blkdev;
    } else {
        block_devices->next->prev = blkdev;
        blkdev->prev = block_devices;
        blkdev->next = block_devices->next;
        block_devices->next = blkdev;
    }
}

BlockDevice* blk_device_get(dev_t dev)
{
    if (!block_devices) return nullptr;
    auto* p = block_devices;
    do {
        if (p->dev == dev) return p;
        p = p->next;
    } while (p != block_devices);
}

CharDevice* chr_device_get(dev_t dev)
{
    if (!char_devices) return nullptr;
    auto* p = char_devices;
    do {
        if (p->dev == dev) return p;
        p = p->next;
    } while (p != char_devices);
}

