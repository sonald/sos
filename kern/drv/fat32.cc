#include <fat32.h>
#include <common.h>
#include <disk.h>
#include <devices.h>
#include <string.h>
#include <vm.h>
#include <ctype.h>

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

// need to handle other invalid chars
static int sanity_name(const char* name, char* res)
{
    int n = strlen(name), i = 0;
    for (i = 0; i < n; i++) {
        if (name[i] >= 'a' && name[i] <= 'z') {
            res[i] = name[i] - 32;
        } else res[i] = name[i];
    }
    res[i] = 0;
    return 0;
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
    ip->attr = std_dp->std.attr;

    if (_type == FatType::Fat32) {
        ip->start_cluster = std_dp->std.start_cluster_lo + (std_dp->std.start_cluster_hi << 16);
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
        //sanity_name(lfn_name, ip->long_name);
        ip->lfn_len = lfn_len;
        memcpy(ip->long_name, lfn_name, lfn_len);
        ip->long_name[lfn_len] = 0;
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

    memcpy(&_fat_bs, &bufp->data, sizeof _fat_bs);
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

    //kprintf("type: %s, _total_sects: %d, _clusters: %d, _fat_sects: %d,"
            //"_root_sects: %d, _data_start_sect: %d, _root_start_sect: %d, "
            //"_lba_start: %d\n",
            //(_type == FatType::Fat32?"fat32":"fat16"), _total_sects,
            //_clusters, _fat_sects, _root_sects, _data_start_sect, _root_start_sect,
            //_lba_start);

    kassert(32 == sizeof(union fat_dirent));

    // create _iroot
    _iroot = vfs.alloc_inode();
    _iroot->ino = 1;
    read_inode(_iroot);

    _icache_size = 0;
    memset(_icaches, 0, sizeof _icaches);
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
void Fat32Fs::read_inode(inode_t *ip, fat_inode_t* fat_ip)
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
        ip->dev = _dev;
        ip->type = (fat_ip->attr & DIRECTORY) ? FsNodeType::Dir : FsNodeType::File;
        ip->size = fat_ip->size; //FIXME: if dir, need traverse to do it !!
        ip->blksize = _blksize;
        ip->blocks = (ip->size + _blksize - 1) / _blksize;
        ip->fs = this;

        ip->data = (void*)fat_ip;
    }
}

static bool str_caseequal(const char* s1, const char* s2)
{
    //kprintf("%s: %s, %s \n", __func__, s1, s2);
    const char* p1 = s1, *p2 = s2;
    while (*p1 && *p2) {
        int c1 = tolower(*p1), c2 = tolower(*p2);
        if (c1 != c2) return false;
        p1++, p2++;
    }

    return *p1 == 0 && *p2 == 0;
}

static bool name_equal(const char* name, fat_inode_t* fat_ip)
{
    if (str_caseequal(fat_ip->std_name, name))
        return true;
    else if (fat_ip->long_name && str_caseequal(fat_ip->long_name, name))
        return true;
    return false;
}

dentry_t* Fat32Fs::lookup(inode_t * dir, dentry_t *de)
{
    scan_dir_option_t opt = { .name = de->name, .target_id = -1 };
    scan_dir(dir, &opt, &de->ip);
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

int Fat32Fs::scan_dir(inode_t* dir, scan_dir_option_t* opt, inode_t** ip)
{
    union fat_dirent* dpp = (union fat_dirent*)vmm.kmalloc(sizeof(union fat_dirent) * 64, 1);
    int dp_len = 0;
    int DPS = BYTES_PER_SECT / sizeof(union fat_dirent);
    fat_inode_t* fat_ip = NULL;
    int entry_id = 0;
    bool end_of_dir = false;

    uint32_t start_sector = 0;
    uint32_t cluster = 0;
    if (dir->ino == FAT_ROOT_INO) {
        start_sector = _root_start_sect;
    } else {
        cluster = ((fat_inode_t*)dir->data)->start_cluster;
        start_sector = cluster2sector(cluster);
    }
    while (1) {
        // kprintf("[%s: sect %d by %s]", __func__, start_sector, (opt->name ? "name" : "id"));
        auto sect_abs = start_sector + _lba_start;
        int count = 0;
        while (count < _fat_bs.sectors_per_cluster) {
            Buffer* bufp = bio.read(_dev, sect_abs + count++);

            union fat_dirent * dp = (union fat_dirent *)&bufp->data;
            for (int i = 0; i < DPS; i++) {
                if (dp[i].lfn.lfn_seq == 0) { // end of dir
                    dp_len = 0;
                    bio.release(bufp);
                    end_of_dir = true;
                    goto _out;

                } else if (dp[i].lfn.lfn_seq == 0xe5) {
                    continue;
                }

                if (dp[i].lfn.attr == LFN) {
                    memcpy(dpp + dp_len++, dp + i, sizeof(union fat_dirent));
                } else {
                    memcpy(dpp + dp_len++, dp + i, sizeof(union fat_dirent));

                    // assemble all info
                    fat_ip = build_fat_inode(dpp, dp_len);
                    fat_ip->dir_id = entry_id;
                    fat_ip->dir_start = ((fat_inode_t*)dir->data)->start_cluster;
                    if (opt->name) {
                        if (name_equal(opt->name, fat_ip)) {
                            bio.release(bufp);
                            goto _out;
                        }
                    } else {
                        kassert(opt->target_id >= 0);
                        if (opt->target_id == entry_id) {
                            bio.release(bufp);
                            goto _out;
                        }
                    }

                    entry_id++;
                    delete fat_ip;
                    fat_ip = 0;
                    dp_len = 0;
                }
            }
            bio.release(bufp);
        }

        if (dir->ino == FAT_ROOT_INO) {
            start_sector++;
            if (start_sector - _root_start_sect >= _root_sects) break;

        } else {
            cluster = find_next_cluster(cluster);
            if (cluster >= CLUSTER_DATA_END_16) break;

            // kprintf("%s: next cluster: %d ", __func__, cluster);
            start_sector = cluster2sector(cluster);
        }
    }

_out:
    vmm.kfree(dpp);

    if (fat_ip) {
        *ip = vfs.alloc_inode();
        (*ip)->ino = fat_ip->start_cluster;
        read_inode(*ip, fat_ip);
        // kprintf("[%s: found %s attr 0x%x type %s]", __func__, fat_ip->std_name,
        //     fat_ip->attr, ((*ip)->type == FsNodeType::Dir ? "dir" : "file"));

    }

    return end_of_dir ? 0 : 1;
}

// off is relative to cluster,
// if count > bytes per cluster, drop remaining
ssize_t Fat32Fs::read_file_cluster(char * buf, size_t count, uint32_t off, uint32_t cluster)
{
    uint32_t sect_abs = cluster2sector(cluster) + _lba_start
        + off / _fat_bs.bytes_per_sector;

    if (count > _blksize - off) count = _blksize - off;
    off_t off_in_sect = off % _fat_bs.bytes_per_sector;
    ssize_t len = count;
    while (len > 0) {
        auto* bufp = bio.read(_dev, sect_abs);
        memcpy(buf, bufp->data + off_in_sect, min(len, _fat_bs.bytes_per_sector - off_in_sect));
        bio.release(bufp);

        len -= (_fat_bs.bytes_per_sector - off_in_sect);
        buf += (_fat_bs.bytes_per_sector - off_in_sect);
        sect_abs++;
        off_in_sect = 0;
    }

    return count;
}

ssize_t Fat32Fs::read(File *filp, char * buf, size_t count, off_t * offset)
{
    auto* ip = filp->inode();
    auto* fat_ip = (fat_inode_t*)ip->data;

    // kassert(ip->type == FsNodeType::File);

    off_t off = *offset;
    if (off >= ip->size) return 0;
    if (count + off >= ip->size) count = ip->size - off;

    uint32_t cluster_order = off / _blksize;
    auto cluster = fat_ip->start_cluster;
    while (cluster_order-- > 0) {
        cluster = find_next_cluster(cluster);
        if (cluster >= CLUSTER_DATA_END_16)
            break;
    }

    uint32_t rel_off = off % _blksize;  // off in current cluster

    ssize_t len = count;
    uint32_t total_read = 0;
    while (len >= 0) {
        uint32_t nr_read = read_file_cluster(buf, len, rel_off, cluster);
        total_read += nr_read;
        rel_off = (off + total_read) % _blksize;
        len -= nr_read;
        buf += nr_read;
        cluster = find_next_cluster(cluster);
        if (cluster >= CLUSTER_DATA_END_16)
            break;
    }

    filp->set_off(filp->off() + count);
    if (offset) *offset = filp->off();

    return count;
}

ssize_t Fat32Fs::write(File *filp, const char * buf, size_t count, off_t *offset)
{
    (void)filp;
    (void)buf;
    (void)count;
    (void)offset;
    return -EINVAL;
}

fat_inode_t* Fat32Fs::find_in_cache(inode_t* dir, int id)
{
    uint32_t dir_start = ((fat_inode_t*)dir->data)->start_cluster;
    for (int i = 0; i < _icache_size; i++) {
        if (_icaches[i].dir_id == id && _icaches[i].dir_start == dir_start) {
            // kprintf("   CACHE     ");
            return &_icaches[i];
        }
    }

    return NULL;
}

void Fat32Fs::push_cache(fat_inode_t* fat_ip)
{
    memcpy(&_icaches[_icache_size], fat_ip, sizeof *fat_ip);
    _icache_size = (_icache_size + 1) % 256;
}

// this is very inefficent
int Fat32Fs::readdir(File *filp, dentry_t *de, filldir_t)
{
    inode_t* dir = filp->inode();
    int id = filp->off() / sizeof(union fat_dirent);
    int ret = 1;
    if (dir->type != FsNodeType::Dir) return -EINVAL;

    fat_inode_t* fat_ip = find_in_cache(dir, id);
    if (fat_ip) {
        de->ip = vfs.alloc_inode();
        de->ip->ino = fat_ip->start_cluster;
        fat_inode_t* new_fip = new fat_inode_t;
        memcpy(new_fip, fat_ip, sizeof *new_fip);
        read_inode(de->ip, new_fip);

    } else {
        // kprintf(" [%s: off %d, id %d] ", __func__, filp->off(), id);
        scan_dir_option_t opt = { .name = NULL, .target_id = id };
        if (scan_dir(dir, &opt, &de->ip) == 0) {
            ret = 0;
        }
    }
    filp->set_off((id+1)*sizeof(union fat_dirent));

    if (de->ip) {
        auto* fat_ip = (fat_inode_t*)de->ip->data;
        push_cache(fat_ip);
        if (fat_ip->long_name) {
            strncpy(de->name, fat_ip->long_name, NAMELEN);
        } else {
            strcpy(de->name, fat_ip->std_name);
        }
    }
    return ret;
}

FileSystem* create_fat32fs(const void* data)
{
    dev_t dev = (dev_t)data;
    auto* fs = new Fat32Fs;
    fs->init(dev);
    return fs;
}
