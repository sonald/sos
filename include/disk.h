#ifndef _SOS_DISK_H
#define _SOS_DISK_H 

// generic hard disk information

#include "types.h"
#include "blkio.h"

#define EXTENDED_PARTITION 5
enum class PartType : uint8_t {
    Fat32 = 0x0B,
    Fat32L = 0x0C, // WIN95 OSR2 FAT32, LBA-mapped
    LinuxSwap  = 0x82,
    Linux  = 0x83
};

typedef struct partition_s {
    uint8_t boot;     /* 0x80 - active */
    uint8_t head;     /* starting head */
    uint8_t sector;   /* starting sector 
                       * & holds bits 8 and 9 of the cyl */
    uint8_t cyl;      /* starting cylinder */
    PartType part_type;    /* What partition type */
    uint8_t end_head;     /* end head */
    uint8_t end_sector;   /* end sector */
    uint8_t end_cyl;      /* end cylinder */
    uint32_t lba_start;   /* starting sector counting from 0 */
    uint32_t nr_sects;    /* nr of sectors in partition */
} partition_t;

#define PART_TABLE_OFFSET 446

// msdos disk
class Disk {
    public:
        void init(dev_t dev);
        int num() const { return _nr_parts; }
        const partition_t* part(int idx) const { return &_parts[idx]; }

    private:
        partition_t _parts[4];
        int _nr_parts {0};
};

#endif
