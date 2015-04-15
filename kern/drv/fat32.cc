#include <fat32.h>
#include <disk.h>
#include <devices.h>
#include <string.h>
#include <vm.h>

// arg name should have enough space
// return size read
static int read_lfn(union fat_dirent* dp, char* name)
{
    int j = 0;
    for (int k = 0; k < 5; k++) {
        if (dp->lfn.name1[k] == 0xFFFF) goto _out;
        name[j++] = dp->lfn.name1[k] & 0xff;
    }
    for (int k = 0; k < 6; k++) {
        if (dp->lfn.name2[k] == 0xFFFF) goto _out;        
        name[j++] = dp->lfn.name2[k] & 0xff;
    }
    for (int k = 0; k < 2; k++) {
        if (dp->lfn.name3[k] == 0xFFFF) goto _out;        
        name[j++] = dp->lfn.name3[k] & 0xff;
    }

_out:    
    name[j] = 0;
    return j;
}

fat_inode_t* Fat32Fs::build_fat_inode(union fat_dirent* dp, int dp_len)
{
    auto* ip = new fat_inode_t;
    memset(ip, 0, sizeof *ip);
    auto* std_dp = &dp[dp_len-1];
    {
        char* name = ip->std_name;
        int i = 0;
        for (i = 0; i < 8; i++) {
            if (std_dp->std.name[i] == ' ') break;
            name[i] = std_dp->std.name[i];
        }

        int j = 0;
        for (j = 0; j < 3; j++) {
            if (std_dp->std.ext[j] == ' ') break;
        }

        if (j) {
            name[i++] = '.';
            strncpy(name+i, std_dp->std.ext, j);
            name[i+3] = 0;
        }
        // kprintf("%s, ", ip->std_name);
    }
    ip->size =std_dp->std.size;
    if (_type == FatType::Fat32) {
        ip->start_cluster = std_dp->std.start_cluster_lo + std_dp->std.start_cluster_hi << 16;
    } else {
        ip->start_cluster = std_dp->std.start_cluster_lo;
        kassert(std_dp->std.start_cluster_hi == 0);
    }

    if (--dp_len) {
        char* lfn_name = new char[64 * 13];
        lfn_name[0] = 0;
        int lfn_len = 0;

        for (int i = dp_len-1; i >= 0; i--) {
            char name[14];
            auto len = read_lfn(dp+i, name);
            strncat(lfn_name + lfn_len, name, len);
            lfn_len += len;
        }

        lfn_len = strlen(lfn_name);
        ip->long_name = new char[lfn_len+1];
        ip->lfn_len = lfn_len;

        strcpy(ip->long_name, lfn_name);        
        delete lfn_name;

        // kprintf("LFN: %s, ", ip->long_name);        
    }

    return ip;
}

void Fat32Fs::init(dev_t dev)
{
    Disk* hd = new Disk;

    _dev = DEVNO(MAJOR(dev), 0);
    int partid = MINOR(dev)-1;
    kprintf("load fat32 at drive %d part %d\n", _dev, partid);
    hd->init(_dev);
    auto* part = hd->part(partid);
    kassert(part->part_type == PartType::Fat32L);

    _lba_start = part->lba_start;
    Buffer* bufp = bio.read(_dev, _lba_start);
    delete hd;

    _fat_bs = *(fat_bs_t*)&bufp->data;
    bio.release(bufp);

    _total_sects = _fat_bs.total_sectors_16 == 0 ?
        _fat_bs.total_sectors_32 : _fat_bs.total_sectors_16;
    _fat_sects = _fat_bs.table_size_16 ?
        _fat_bs.table_size_16 : _fat_bs.ebs32.table_size_32;
    //for fat16/12
    _root_sects = (_fat_bs.root_entry_count * sizeof(union fat_dirent)
         + _fat_bs.bytes_per_sector - 1) / _fat_bs.bytes_per_sector;
    _data_start_sect = _fat_bs.reserved_sector_count +
        _fat_bs.table_count * _fat_sects + _root_sects;

    _clusters = (_total_sects - _data_start_sect) / _fat_bs.sectors_per_cluster;
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
        _root_start_sect = cluster2sector(_fat_bs.ebs32.root_cluster);        
    }
    _blksize = _fat_bs.sectors_per_cluster * _fat_bs.bytes_per_sector;

    if (_type == FatType::Fat32) {
        _fat_bs.ebs32.volume_label[10] = 0;
        _fat_bs.ebs32.fat_type_label[7] = 0;
    } else {
        _fat_bs.ebs16.volume_label[10] = 0;
        _fat_bs.ebs16.fat_type_label[7] = 0;
    }

    uint32_t fat_start_sect = _fat_bs.reserved_sector_count + _lba_start;
    bufp = bio.read(_dev, fat_start_sect);
    uint16_t* fat_data = (uint16_t*)&bufp->data;
    kassert(*fat_data == (0xff00 | _fat_bs.media_type));
    // kassert(*(fat_data + 1) >= 0x3fff);
    bio.release(bufp);

    // kprintf("fat: oem %s, BPS: %d, SPC: %d, RSC: %d, "
    //         "TC: %d, REC: %d, total sect16: %d, total sect32: %d\n"
    //         "tbl sz16: %d, drive: %d, "
    //         "vol: %s, type: %s, total _clusters: %d\n",
    //         _fat_bs.oem_name, _fat_bs.bytes_per_sector,
    //         _fat_bs.sectors_per_cluster, _fat_bs.reserved_sector_count,
    //         _fat_bs.table_count, _fat_bs.root_entry_count,
    //         _fat_bs.total_sectors_16, _fat_bs.total_sectors_32,
    //         _fat_bs.table_size_16,
    //         _fat_bs.ebs16.bios_drive_num, _fat_bs.ebs16.volume_label,
    //         _fat_bs.ebs16.fat_type_label, _clusters);
    kprintf("type: %s, _total_sects: %d, _clusters: %d, _fat_sects: %d,"
            "_root_sects: %d, _data_start_sect: %d, _root_start_sect: %d, "
            "_lba_start: %d\n",
            (_type == FatType::Fat32?"fat32":"fat16"), _total_sects,
            _clusters, _fat_sects, _root_sects, _data_start_sect, _root_start_sect,
            _lba_start);
            
    kassert(32 == sizeof(union fat_dirent));

    // create _iroot
    _iroot = vfs.alloc_inode();
    _iroot->ino = 1;
    read_inode(_iroot, 0);
}

uint32_t Fat32Fs::sector2cluster(uint32_t sect)
{
    return (sect - _data_start_sect) / _fat_bs.sectors_per_cluster + 2;
}

uint32_t Fat32Fs::cluster2sector(uint32_t cluster)
{
    return _data_start_sect + (cluster - 2) * _fat_bs.sectors_per_cluster;
}

//how ino map to fat file: right now use dynamical map
void Fat32Fs::read_inode(inode_t *ip, union fat_dirent* fat_de)
{
    if (ip->ino == 1) {
        ip->dev = _dev;
        ip->type = FsNodeType::Dir;
        ip->size = 0;
        ip->blksize = _blksize; 
        ip->blocks = (ip->size + _blksize - 1) / _blksize;
        ip->fs = this;

        fat_inode_t* fat_ip = new fat_inode_t;
        memset(fat_ip, 0, sizeof *fat_ip);
        ip->data = (void*)fat_ip;
        // for fat12/16, this is invalid
        fat_ip->start_cluster = sector2cluster(_root_start_sect); // care about fat32
    } else {
        kassert("can not be used for other than root");
    }
}

// need to handle other invalid chars
static int sanity_name(const char* name, char* res) 
{
    int n = strlen(name);
    for (int i = 0; i < n; i++) {
        if (name[i] >= 'a' && name[i] <= 'z') {
            res[i] = name[i] - 32;
        } else res[i] = name[i];
    }
    return 0;
}

static bool name_equal(const char* name, fat_inode_t* fat_ip) 
{
    if (strcmp(fat_ip->std_name, name) == 0) 
        return true;
    else if (fat_ip->long_name && strcmp(fat_ip->long_name, name) == 0)
        return true;
    return false;
}

dentry_t* Fat32Fs::lookup(inode_t * dir, dentry_t *de)
{
    char name[sizeof de->name] = "";
    sanity_name(de->name, name);

    if (dir->ino == FAT_ROOT_INO) {
        de->ip = scan_root_dir(dir, name);
    } else {
        de->ip = scan_non_root_dir(dir, name);    
    }    

    return de;
}
    
//FIXME: fat16 only!
//TODO: cache whole FAT in memory
uint32_t Fat32Fs::find_next_cluster(uint32_t cluster)
{
    int cps = _fat_bs.bytes_per_sector / sizeof (uint16_t);
    uint32_t fat_start_sect = _fat_bs.reserved_sector_count + _lba_start + cluster / cps;
    auto* bufp = bio.read(_dev, fat_start_sect);
    uint16_t* fat_data = (uint16_t*)&bufp->data;
    uint32_t next = *(fat_data + cluster % cps);
    bio.release(bufp);
    return next;
}

inode_t* Fat32Fs::scan_non_root_dir(inode_t* dir, const char* name)
{
    kprintf("[%s: %s] ", __func__, name);

    auto cluster = ((fat_inode_t*)dir->data)->start_cluster;   
    auto start_sect = cluster2sector(cluster);
    inode_t* ip = NULL;
    while (scan_dir_cluster(dir, name, start_sect, &ip) > 0) {
        if (ip) return ip;

        cluster = find_next_cluster(cluster);        
        if (cluster >= CLUSTER_DATA_END_16) break;

        kprintf("%s: next cluster: %d ", cluster);
        start_sect = cluster2sector(cluster);
    }
    return ip;
}

inode_t* Fat32Fs::scan_root_dir(inode_t* dir, const char* name)
{
    kprintf("[%s: %s] ", __func__, name);
    uint32_t n = 0;
    inode_t* ip = NULL;
    while (scan_dir_cluster(dir, name, _root_start_sect + n, &ip) > 0) {
        if (ip) return ip;
        if (n++ >= _root_sects) break;
    }
    return ip;
}

// return <= 0 end of dir
int Fat32Fs::scan_dir_cluster(inode_t* dir, const char* name, uint32_t start_sector, inode_t** ip)
{
    kprintf("%s: %s sect %d ", __func__, name, start_sector);
    auto sect_abs = start_sector + _lba_start;
    int count = 0;
    union fat_dirent* dpp = (union fat_dirent*)vmm.kmalloc(sizeof(union fat_dirent) * 64, 1);
    int dp_len = 0;
    bool done = false; // end of dir
    int DPS = BYTES_PER_SECT / sizeof(union fat_dirent);

    fat_inode_t* fat_ip = NULL;

    while (count < _fat_bs.sectors_per_cluster && !done) {
        Buffer* bufp = bio.read(_dev, sect_abs + count++);

        union fat_dirent * dp = (union fat_dirent *)&bufp->data;
        for (int i = 0; i < DPS; i++) {
            if (dp[i].lfn.lfn_seq == 0) { // end of dir
                dp_len = 0;
                done = true;
                break;
            }

            if (dp[i].lfn.lfn_seq == 0xe5) { continue; }

            if (dp[i].lfn.attr == LFN) {
                memcpy(dpp + dp_len++, dp + i, sizeof(union fat_dirent));
            } else {
                memcpy(dpp + dp_len++, dp + i, sizeof(union fat_dirent));                    
                // assemble all info
                fat_ip = build_fat_inode(dpp, dp_len);
                if (name_equal(name, fat_ip)) {
                    bio.release(bufp);
                    goto _out;
                }

                delete fat_ip;
                fat_ip = 0;
                dp_len = 0;
            }
        }
        bio.release(bufp);
    }

_out:
    vmm.kfree(dpp);

    if (fat_ip) {
        *ip = vfs.alloc_inode();
        (*ip)->dev = _dev;
        (*ip)->type = (fat_ip->attr & DIRECTORY) ? FsNodeType::Dir : FsNodeType::File;
        (*ip)->size = fat_ip->size; //FIXME: if dir, need traverse to do it !!
        (*ip)->blksize = _blksize; 
        (*ip)->blocks = ((*ip)->size + _blksize - 1) / _blksize;
        (*ip)->fs = this;

        (*ip)->data = (void*)fat_ip;
    }

    return done ? 0:1;
}

//this is wrong
ssize_t Fat32Fs::read(File *filp, char * buf, size_t count, off_t * offset)
{
    auto* ip = filp->inode();
    auto* fat_ip = (fat_inode_t*)ip->data;

    kassert(ip->type == FsNodeType::File);

    off_t off = *offset;
    if (off >= ip->size) return 0;
    if (count + off >= ip->size) count = ip->size - off;

        // cluster = find_next_cluster(cluster);
        // if (cluster >= CLUSTER_DATA_END_16) break;

    uint32_t sect_abs = cluster2sector(fat_ip->start_cluster) + _lba_start 
        + off / _fat_bs.bytes_per_sector;
    off_t off_in_sect = off % _fat_bs.bytes_per_sector;
    ssize_t len = count; // need signed value
    while (len > 0) {
        auto* bufp = bio.read(_dev, sect_abs);
        memcpy(buf, bufp->data + off_in_sect, _fat_bs.bytes_per_sector);        
        len -= (_fat_bs.bytes_per_sector - off_in_sect);
        buf += (_fat_bs.bytes_per_sector - off_in_sect);
        sect_abs++;
        bio.release(bufp);

        off_in_sect = 0;
    }

    filp->set_off(off + count);
    if (offset) *offset = filp->off();

    return count;
}

ssize_t Fat32Fs::write(File *filp, const char * buf, size_t count, off_t *offset)
{
    (void)filp;
    (void)buf;
    (void)count;
    (void)offset;
    return 0;
}

int Fat32Fs::readdir(File *filp, dentry_t *de, filldir_t)
{
    inode_t* ip = filp->inode();
    int id = filp->off() / sizeof(union fat_dirent);

    if (ip->type != FsNodeType::Dir) return -EINVAL;

    // initrd_entry_header_t* ieh = &_sb->files[id];
    // strcpy(de->name, ieh->name);
    
    de->ip = vfs.alloc_inode();
    // de->ip->ino = id+2;
    // read_inode(de->ip);

    filp->set_off((id+1)*sizeof(union fat_dirent));
    return 0;
}

FileSystem* create_fat32fs(const void* data)
{
    dev_t dev = (dev_t)data;
    auto* fs = new Fat32Fs;
    fs->init(dev);
    return fs;
}
