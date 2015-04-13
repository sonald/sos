#ifndef _SOS_FAT32_H
#define _SOS_FAT32_H 

#include <vfs.h>

// Spec:
// http://en.wikipedia.org/wiki/Design_of_the_FAT_file_system

typedef struct fat_extBS_32
{
    //extended fat32 stuff
    unsigned int        table_size_32;
    unsigned short      extended_flags;
    unsigned short      fat_version;
    unsigned int        root_cluster;
    unsigned short      fat_info;
    unsigned short      backup_BS_sector;
    unsigned char       reserved_0[12];
    unsigned char       drive_number;
    unsigned char       reserved_1;
    unsigned char       boot_signature;
    unsigned int        volume_id;
    unsigned char       volume_label[11];
    unsigned char       fat_type_label[8];
 
}__attribute__((packed)) fat_extBS_32_t;
 
typedef struct fat_extBS_16
{
    //extended fat12 and fat16 stuff
    unsigned char       bios_drive_num;
    unsigned char       reserved1;
    unsigned char       boot_signature;
    unsigned int        volume_id;
    unsigned char       volume_label[11];
    unsigned char       fat_type_label[8];
 
}__attribute__((packed)) fat_extBS_16_t;
 
//Bios Parameter Block
typedef struct fat_bs
{
    unsigned char       bootjmp[3];
    unsigned char       oem_name[8];
    unsigned short      bytes_per_sector;
    unsigned char       sectors_per_cluster;
    unsigned short      reserved_sector_count;
    unsigned char       table_count;
    unsigned short      root_entry_count;
    unsigned short      total_sectors_16;
    unsigned char       media_type;
    unsigned short      table_size_16;
    unsigned short      sectors_per_track;
    unsigned short      head_side_count;
    unsigned int        hidden_sector_count;
    unsigned int        total_sectors_32;
 
    //this will be cast to it's specific type once the driver actually knows what type of FAT this is.
    //unsigned char       extended_section[54];
    union {
        fat_extBS_16_t ebs16;
        fat_extBS_32_t ebs32;
    };
 
}__attribute__((packed)) fat_bs_t;

class Fat32Fs: public FileSystem {
    public:
        enum class FatType {
            Fat32,
            Fat16,
            Fat12
        };

        enum {
            READ_ONLY=0x01,
            HIDDEN=0x02, 
            SYSTEM=0x04, 
            VOLUME_ID=0x08, 
            DIRECTORY=0x10, 
            ARCHIVE=0x20,
            LFN=READ_ONLY|HIDDEN|SYSTEM|VOLUME_ID 
        };

        union dirent {
            struct {
                char name[11]; // 8.3 name
                uint8_t attr;
                uint8_t reserved;
                uint8_t create_tenth;
                uint16_t create_time;
                uint16_t create_date;
                uint16_t access_date;
                uint16_t start_cluster_hi; // for fat16/12 is 0
                uint16_t mod_time;
                uint16_t mod_date;
                uint16_t start_cluster_lo; 
                uint32_t size; // in bytes
            } std;  // standard 8.3 format

            struct {
                uint8_t order;
                char name1[10];
                uint8_t attr;
                uint8_t type; // 0
                uint8_t checksum;
                char name2[12];
                uint16_t zeroed; // 0
                char name3[4];
            } lfn;  // long file names
        } __attribute__((packed));

        void init(dev_t dev);

    private:
        dev_t _dev;
        fat_bs_t _fat_bs;
        FatType _type;

        uint32_t _total_sects;
        uint32_t _fat_sects;
        uint32_t _root_sects; // fat12/16
        uint32_t _root_start_sect;
        uint32_t _data_start_sect;
        uint32_t _clusters;

        uint32_t _lba_start; // start phy sector of filesystem in disk
};

FileSystem* create_fat32fs(const void*);
#endif


