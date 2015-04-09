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
    unsigned short          bytes_per_sector;
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
        void init(dev_t dev);
        int read(inode_t* ip, void* buf, size_t nbyte, u32 offset) override;
        int write(inode_t* ip, void* buf, size_t nbyte, u32 offset) override;
        inode_t* dir_lookup(inode_t* ip, const char* name) override;
        dentry_t* dir_read(inode_t* ip, int id) override;

    private:
        dev_t _dev;
};

FileSystem* create_fat32fs(void*);
#endif


