#include <disk.h>
#include <common.h>
#include <string.h>

void Disk::init(dev_t dev)
{
    Buffer* mbr = bio.read(dev, 0);
    kassert(mbr);
    kassert(mbr->data[0x1fe] == 0x55 && mbr->data[0x1ff] == 0xaa);
    memcpy(_parts, &mbr->data[PART_TABLE_OFFSET], sizeof _parts);
    bio.release(mbr);

    kprintf("\nPARTs: \n");
    auto* p = &_parts[0];
    while (p->nr_sects) {
        kprintf("start: 0x%x, sz: 0x%x, b: %d, ty: %s\n", p->lba_start,
                p->nr_sects, p->boot, 
                p->part_type == PartType::Fat32L ? "fat32l"
                : (p->part_type == PartType::Linux ? "ext2" : "(X"));
        p++;
    }
    kputchar('\n');
}

