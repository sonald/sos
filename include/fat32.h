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

// fat_dirent.lfn.lfn_seq
#define LFN_SEQ_NUM(n)  ((n) & 0x3f)
#define LFN_LAST(n)     ((n) & 0x40)
#define LFN_ERASED(n)   ((n) & 0x80)

enum FatAttr {
    READ_ONLY=0x01,
    HIDDEN=0x02, 
    SYSTEM=0x04, 
    VOLUME_ID=0x08, 
    DIRECTORY=0x10, 
    ARCHIVE=0x20,
    LFN=READ_ONLY|HIDDEN|SYSTEM|VOLUME_ID 
};

union fat_dirent {
    struct {
        char name[8];  // 8.3 name and ext
        char ext[3];
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
        uint32_t size; // in bytes, only for file, not dir
    } __attribute__((packed)) std;  // standard 8.3 format

    struct {
        uint8_t lfn_seq;
        uint16_t name1[5];
        uint8_t attr;
        uint8_t type; // 0
        uint8_t checksum;
        uint16_t name2[6];
        uint16_t zeroed; // 0
        uint16_t name3[2];
    } __attribute__((packed)) lfn;  // long file names
};

#define FAT_ROOT_INO  1

// fat has no inode, so I come up with a simple idea by mapping 
// inode_t.ino to start_cluster of itself.
// root inode is special 1
// inode_t.data part
typedef struct fat_inode_s {
    uint32_t start_cluster;
    uint32_t size;
    char* long_name;
    uint8_t attr;
    int lfn_len;
    char std_name[13];
} fat_inode_t;

/*
 *             FAT12    FAT16       FAT32
 * Available   000      0000        00000000
 * Reserved    001      0001        00000001
 * User Data   002-FF6  0002-FFF6   00000002-0FFFFFF6
 * Bad Cluster FF7      FFF7        0FFFFFF7
 * End Marker  FF8-FFF  FFF8-FFFF   0FFFFFF8-0FFFFFFF
 **/
enum ClusterStatus {
    CLUSTER_AVAIL           = 0x0,
    CLUSTER_RESERVED        = 0x01,
    CLUSTER_DATA_BEGIN      = 0x02,
    CLUSTER_DATA_END_12     = 0x0ff7,
    CLUSTER_DATA_END_16     = 0x0fff7,     
    CLUSTER_DATA_END_32     = 0x0ffffff7, 
};

class Fat32Fs: public FileSystem {
    public:
        enum class FatType {
            Fat32,
            Fat16,
            Fat12
        };

        void init(dev_t dev);
        dentry_t * lookup(inode_t * dir, dentry_t *) override;

        ssize_t read(File *, char * buf, size_t count, off_t * offset) override;
        ssize_t write(File *, const char * buf, size_t, off_t *offset) override;
        int readdir(File *, dentry_t *, filldir_t) override;

    private:
        dev_t _dev;  // disk device, no partition info included
        fat_bs_t _fat_bs;
        FatType _type;

        uint32_t _total_sects;
        uint32_t _fat_sects;
        uint32_t _root_sects; // fat12/16
        uint32_t _root_start_sect;
        uint32_t _data_start_sect;
        uint32_t _clusters;
        uint32_t _blksize;
        uint32_t _lba_start; // start phy sector of filesystem in disk

        uint32_t sector2cluster(uint32_t);
        uint32_t cluster2sector(uint32_t);
        void read_inode(inode_t* ip); 
        inode_t* scan_non_root_dir(inode_t* dir, const char* name);
        inode_t* scan_root_dir(inode_t* dir, const char* name);    

        int scan_dir_cluster(inode_t* dir, const char* name, uint32_t start_sector, inode_t** ip);
        ssize_t read_file_cluster(char * buf, size_t count, uint32_t off, uint32_t cluster);
        fat_inode_t* build_fat_inode(union fat_dirent* dp, int dp_len);
        uint32_t find_next_cluster(uint32_t cluster);        
};

FileSystem* create_fat32fs(const void*);
#endif


