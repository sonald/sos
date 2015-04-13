#include <fat32.h>
#include <disk.h>
#include <devices.h>
#include <string.h>

void Fat32Fs::init(dev_t dev)
{
    Disk* hd = new Disk;

    this->_dev = dev;
    int partid = MINOR(dev)-1;
    dev_t hdid = DEVNO(MAJOR(dev), 0);
    kprintf("load fat32 at drive %d part %d\n", hdid, partid);
    hd->init(hdid);
    auto* part = hd->part(partid);
    kassert(part->part_type == PartType::Fat32L);

    _lba_start = part->lba_start;
    Buffer* bufp = bio.read(hdid, part->lba_start);
    delete hd;

    _fat_bs = *(fat_bs_t*)&bufp->data;
    bio.release(bufp);

    uint32_t _total_sects = _fat_bs.total_sectors_16 == 0 ?
        _fat_bs.total_sectors_32 : _fat_bs.total_sectors_16;
    uint32_t _fat_sects = _fat_bs.table_size_16 ?
        _fat_bs.table_size_16 : _fat_bs.ebs32.table_size_32;
    //for fat16/12
    uint32_t _root_sects =
        (_fat_bs.root_entry_count * sizeof(union dirent)
         + _fat_bs.bytes_per_sector - 1) / _fat_bs.bytes_per_sector;
    uint32_t _data_start_sect =
        _fat_bs.reserved_sector_count +
        _fat_bs.table_count * _fat_sects + _root_sects;

    uint32_t _clusters = (_total_sects - _data_start_sect) / _fat_bs.sectors_per_cluster;
    if (_clusters < 4085) {
        _type = FatType::Fat12;
    } else if (_clusters < 65525) {
        _type = FatType::Fat16;
    } else {
        _type = FatType::Fat32;
    }

    if (_type != FatType::Fat32) {
        _root_start_sect = _fat_bs.reserved_sector_count + _fat_bs.table_count * _fat_sects;
    } else {
        _root_start_sect = _data_start_sect + (_fat_bs.ebs32.root_cluster - 2)* _fat_bs.sectors_per_cluster;
    }

    if (_type == FatType::Fat32) {
        _fat_bs.ebs32.volume_label[10] = 0;
        _fat_bs.ebs32.fat_type_label[7] = 0;
    } else {
        _fat_bs.ebs16.volume_label[10] = 0;
        _fat_bs.ebs16.fat_type_label[7] = 0;
    }

    kprintf("fat: oem %s, BPS: %d, SPC: %d, RSC: %d, "
            "TC: %d, REC: %d, total sect16: %d, total sect32: %d\n"
            "tbl sz16: %d, drive: %d, "
            "vol: %s, type: %s, total _clusters: %d\n",
            _fat_bs.oem_name, _fat_bs.bytes_per_sector,
            _fat_bs.sectors_per_cluster, _fat_bs.reserved_sector_count,
            _fat_bs.table_count, _fat_bs.root_entry_count,
            _fat_bs.total_sectors_16, _fat_bs.total_sectors_32,
            _fat_bs.table_size_16,
            _fat_bs.ebs16.bios_drive_num, _fat_bs.ebs16.volume_label,
            _fat_bs.ebs16.fat_type_label, _clusters);
    kprintf("type: %s, _total_sects: %d, _clusters: %d, _fat_sects: %d,"
            "_root_sects: %d, _data_start_sect: %d\n",
            (_type == FatType::Fat32?"fat32":"fat16"), _total_sects,
            _clusters, _fat_sects, _root_sects, _data_start_sect);

    if (_type == FatType::Fat16) {
        // read root dir
        Buffer* bufp = bio.read(hdid, _lba_start + _root_start_sect);
        int n = BYTES_PER_SECT / sizeof(union dirent);
        union dirent * dp = (union dirent *)&bufp->data;
        for (int i = 0; i < n; i++) {
            if (dp[i].lfn.attr == LFN) {
                char name[14];
                int j = 0;
                for (int k = 0; k < 5; k++) {
                    name[j++] = dp[i].lfn.name1[k*2];
                }
                for (int k = 0; k < 6; k++) {
                    name[j++] = dp[i].lfn.name2[k*2];
                }
                for (int k = 0; k < 2; k++) {
                    name[j++] = dp[i].lfn.name3[k*2];
                }
                name[j] = 0;
                kprintf("LFN: %s\t", name);
            } else {
                char name[13];
                strncpy(name, dp[i].std.name, 8);
                name[8] = '.';
                strncpy(name+8, dp[i].std.name+8, 3);
                name[12] = 0;
                kprintf("NRM: %s\t", name);
            }
        }
        bio.release(bufp);

    }
}

FileSystem* create_fat32fs(const void* data)
{
    dev_t dev = (dev_t)data;
    auto* fs = new Fat32Fs;
    fs->init(dev);
    return fs;
}
