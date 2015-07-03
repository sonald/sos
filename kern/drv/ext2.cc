#include <ext2.h>
#include <string.h>
#include <disk.h>
#include <vm.h>
#include <spinlock.h>

#define ROUNDUP(sz, align) (((sz) + (align) - 1) / (align))
#define NR_CACHED_INODES 128

static Spinlock ext2lock {"ext2"};
//static ext2_inode_t ext2_inode_caches[NR_CACHED_INODES];
#define BLK2SECT(bid) ((bid) * _fs.sectors_per_block + _fs.start_sector)

//fast check if id is powers of 3, 5, 7
static int ispow(uint32_t id)
{
    // pows range from 3^1 .... 7^10
    uint32_t pows[] = {
        3, 5, 7, 9, 25, 27, 49, 81, 125, 243, 343, 625, 729, 2187, 2401,
        3125, 6561, 15625, 16807, 19683, 59049, 78125, 117649, 390625,
        823543, 1953125, 5764801, 9765625, 40353607, 282475249
    };

    int l = 0, r = sizeof(pows)/sizeof(pows[0])-1, m;
    while (l <= r) {
        m = (l+r)/2;
        if (pows[m] == id) return 1;
        else if (pows[m] > id) {
            r = m-1;
        } else l = m+1;
    }
    return 0;
}

static inline bool is_regular(ext2_inode_t* eip)
{
    return eip->mode & EXT2_S_IFREG;
}

static inline bool is_dir(ext2_inode_t* eip)
{
    return eip->mode & EXT2_S_IFDIR;
}

static loff_t isize(ext2_inode_t* eip, ext2_superblock_t* sb)
{
    loff_t sz = eip->size;
    if (sb->rev_level >= 1) sz = eip->size | ((loff_t)eip->dir_acl << 32);
    return sz;
}

uint32_t Ext2Fs::inode_occupied_blocks(ext2_inode_t* eip)
{
    return ROUNDUP(eip->sectors, _fs.sectors_per_block);
}

void Ext2Fs::init(dev_t dev)
{
    Disk* hd = new Disk;

    _dev = DEVNO(MAJOR(dev), 0);
    _pdev = dev;
    int partid = MINOR(dev)-1;
    hd->init(_dev);
    auto* part = hd->part(partid);
    kassert(part->part_type == PartType::Linux);

    uint32_t start_sector = part->lba_start;
    Buffer* bufp = bio.read(_dev, start_sector+2);
    memcpy((char*)&_sb, &bufp->data, BYTES_PER_SECT);
    bio.release(bufp);

    bufp = bio.read(_dev, start_sector+3);
    memcpy((char*)&_sb + BYTES_PER_SECT, &bufp->data, BYTES_PER_SECT);
    bio.release(bufp);

    delete hd;

    _fs.groups_count = ROUNDUP(_sb.blocks_count, _sb.blocks_per_group);
    _fs.block_size = 1024 << _sb.log_block_size;
    _fs.frag_size = 1024 << _sb.log_frag_size;
    _fs.sectors_per_block = _fs.block_size / BYTES_PER_SECT;
    _fs.inodes_per_sector = BYTES_PER_SECT / EXT2_INODE_SIZE(_sb);
    _fs.start_sector = start_sector;

    uint32_t scale = _fs.bids_in_blk = _fs.block_size / sizeof(uint32_t);
    _fs.ind_block_max_bid = scale + EXT2_NDIR_BLOCKS;;
    _fs.dind_block_max_bid = scale * scale + _fs.ind_block_max_bid;
    _fs.tind_block_max_bid = scale * scale * scale + _fs.dind_block_max_bid;

    kprintf(""
        "Filesystem volume name:   %s\tLast mounted on:           %s\n"
        "Filesystem UUID:          %s\tFilesystem magic number:  0x%x\n"
        "Filesystem revision #:    %d\tFilesystem features:      ()\n"
        "Filesystem flags:           \tDefault mount options:    \n"
        "Filesystem state:         %s\tErrors behavior:          %d\n"
        "Filesystem OS type:       %d\tInode count:              %d\n"
        "Block count:              %d\tReserved block count:     %d\n"
        "Free blocks:              %d\tFree inodes:              %d\n"
        "First block               %d\tBlock size:               %d\n"
        "Fragment size:            %d\tReserved GDT blocks:      %d\n"
        "Blocks per group:         %d\tFragments per group:      %d\n"
        "Inodes per group:         %d\tInode blocks per group:   %d\n"
        "Filesystem created:       %d\tLast mount time:          %d\n"
        "Last write time:          %d\tMount count               %d\n"
        "Maximum mount count:      %d\tLast checked:             %d\n"
        "Check interval:           %d\tReserved blocks uid:      %d\n"
        "Reserved blocks gid:      %d\tFirst inode:              %d\n"
        "Inode size:               %d\tgroup count:              %d\n",
        _sb.volume_name, _sb.last_mounted,  _sb.uuid, _sb.magic,
        _sb.rev_level, (_sb.state == EXT2_VALID_FS ?"clean":"error"),
        _sb.errors, _sb.creator_os, _sb.inodes_count, _sb.blocks_count,
        _sb.r_blocks_count, _sb.free_blocks_count, _sb.free_inodes_count,
        _sb.first_data_block, _fs.block_size, _fs.frag_size,
        _fs.groups_count * sizeof(ext2_group_desc_t), 
        _sb.blocks_per_group, _sb.frags_per_group, _sb.inodes_per_group,
        -1, /* calculated from gdt */
        _sb.mtime, _sb.mtime, _sb.wtime, 
        _sb.mnt_count, _sb.max_mnt_count, _sb.lastcheck, _sb.checkinterval,
        _sb.def_resuid, _sb.def_resgid, EXT2_FIRST_INO(_sb),
        EXT2_INODE_SIZE(_sb), _fs.groups_count);


    int id = 0;
    int sz = sizeof(ext2_group_desc_t) * _fs.groups_count;
    int sectors = ROUNDUP(sz, BYTES_PER_SECT);
    int gdt_start_sector = start_sector 
        + (_fs.block_size == 1024 ? 4: _fs.sectors_per_block);

    /*
     The first version of ext2 (revision 0) stores a copy at the start
     of every block group, along with backups of the group descriptor 
     block(s). Because this can consume a considerable amount of space
     for large filesystems, later revisions can optionally reduce the
     number of backup copies by only putting backups in specific groups 
     (this is the sparse superblock feature). The groups chosen are 0,
     1 and powers of 3, 5 and 7.
     */
    _gdt = new ext2_group_info_t[_fs.groups_count];
    for (int i = 0; i < sectors; i++) {
        bufp = bio.read(_dev, gdt_start_sector++);

        int gd_sz = sizeof(ext2_group_desc_t);
        for (int j = 0, len = min(sz, BYTES_PER_SECT)/gd_sz; j < len; j++) {
            memcpy((char*)&_gdt[id].desc, (char*)&bufp->data+gd_sz*j, gd_sz);

            _gdt[id].group_no = id;
            _gdt[id].start_block = id * _sb.blocks_per_group 
                + (_fs.block_size == 1024 ? 1: 0);
            _gdt[id].end_block = _gdt[id].start_block + _sb.blocks_per_group - 1;
            _gdt[id].free_end_ino = (id+1) * _sb.inodes_per_group;
            _gdt[id].free_start_ino = _gdt[id].free_end_ino - _gdt[id].desc.free_inodes_count + 1;

            _gdt[id].has_backup_sb = (_sb.rev_level < 1) 
                || id == 0 || id == 1 || ispow(id);
            id++;
        }

        sz -= BYTES_PER_SECT;
        bio.release(bufp);
    }

    _iroot = vfs.alloc_inode(_pdev, EXT2_ROOT_INO);
    kassert(_iroot->ref == 0);
    _iroot->ino = EXT2_ROOT_INO;
    _iroot->ref = 1;
    read_inode(_iroot);

    //for (int i = 0; i < 2; i++) {
        //ext2_group_desc_t& gd = _gdt[i].desc;
        //kprintf("#%d(%d-%d):   bbitmap:  %d\tibitmap    %d\n"
                //"inode table:  %d\tfree blocks:  %d\tfree inodes: %d\n"
                //"used dirs:  %d\tfree inodes: %d - %d, backup  %d\n",
                //i, _gdt[i].start_block, _gdt[i].end_block,
                //gd.block_bitmap, gd.inode_bitmap, gd.inode_table,
                //gd.free_blocks_count, gd.free_inodes_count, 
                //gd.used_dirs_count, 
                //_gdt[i].free_start_ino, _gdt[i].free_end_ino, 
                //_gdt[i].has_backup_sb);
    //}

    ext2_inode_t* eip = (ext2_inode_t*)_iroot->data;
    kprintf("rootno(%d): mode %x, uid %d, gid %d, sz %d, sects %d, "
            "indexed %d\n",
            _iroot->ino, eip->mode, eip->uid, eip->gid, eip->size, 
            eip->sectors, eip->flags & EXT2_INDEX_FL);
}

dentry_t * Ext2Fs::lookup(inode_t* dirp, dentry_t *de) 
{
    if (dirp->type != FsNodeType::Dir) return NULL;
    
    ext2_inode_t* deip = (ext2_inode_t*)dirp->data;
    if (deip->flags & EXT2_INDEX_FL) {
        kprintf("WARN: indexed dir dont supported yet\n");
    }
    
    char* buf = new char[sizeof(ext2_dirent_t)+255];
    uint32_t off = 0;

    while (off < deip->size) {
        auto oflags = ext2lock.lock();
        iread(deip, off, buf, sizeof(ext2_dirent_t)+255);
        ext2lock.release(oflags);

        ext2_dirent_t* dirent = (ext2_dirent_t*)buf;
        //kprintf("%s: ino %d, rec_len: %d, namelen: %d\n",
                //__func__, dirent->inode, dirent->rec_len, dirent->name_len);
        //notice that rec_len could be padded to end of block
        if (dirent->inode == 0) {
            break;
        }
        if (strncmp(de->name, dirent->name, dirent->name_len) == 0) {
            de->ip = vfs.alloc_inode(_pdev, dirent->inode);
            if (de->ip->ref == 0) {
                de->ip->ref = 1;
                de->ip->ino = dirent->inode;
                read_inode(de->ip);
            }
            delete buf;
            return de;
        }
        off += dirent->rec_len;
    }
    delete buf;
    return de;
}

ssize_t Ext2Fs::read(File* filp, char * buf, size_t count, off_t * offset)
{
    auto* ip = filp->inode();
    ext2_inode_t* eip = (ext2_inode_t*)ip->data;
    if (is_regular(eip) && (eip->flags & EXT2_INDEX_FL)) {
        kprintf("WARN: indexed dir dont supported yet\n");
    }

    off_t off = *offset;
    if ((uint32_t)off >= eip->size) return 0;
    if (count + off >= eip->size) {
        count = eip->size - off;
    }
    
    auto oflags = ext2lock.lock();
    iread(eip, off, buf, count);
    filp->set_off(off + count);
    *offset = filp->off();
    ext2lock.release(oflags);

    return count;
}

ssize_t Ext2Fs::write(File* filp, const char * buf, size_t count, off_t *offset)
{
    panic("not implemented");
    return 0;
}

int Ext2Fs::read_whole_block(uint32_t bid, char* buf)
{
    uint32_t sect = BLK2SECT(bid);
    uint32_t end_sect = sect + _fs.block_size / BYTES_PER_SECT - 1;
    uint32_t sz = 0;
    while (sect <= end_sect) {
        Buffer* bufp = bio.read(_dev, sect);
        memcpy(buf + sz, bufp->data, BYTES_PER_SECT);
        bio.release(bufp);
        sz += BYTES_PER_SECT;
        sect++;
    }
    return 0; 
}

// bid is relative to eip
uint32_t Ext2Fs::iget_indirect_block_no(ext2_inode_t* eip, uint32_t bid)
{
    uint32_t ret = 0;
    uint32_t blk_no = 0, scale = 0;

    uint32_t* blk_buf = new uint32_t[_fs.bids_in_blk];

    if (bid >= _fs.dind_block_max_bid) {
        blk_no = eip->block[EXT2_TIND_BLOCK];
        bid -= _fs.dind_block_max_bid;
        scale = _fs.bids_in_blk * _fs.bids_in_blk;
    } else if (bid >= _fs.ind_block_max_bid) {
        blk_no = eip->block[EXT2_DIND_BLOCK];
        bid -= _fs.ind_block_max_bid;
        scale = _fs.bids_in_blk;
    } else if (bid >= EXT2_NDIR_BLOCKS) {
        blk_no = eip->block[EXT2_IND_BLOCK];
        bid -= EXT2_NDIR_BLOCKS;
        scale = 1;
    } else return eip->block[bid];

    while (scale > 0) {
        read_whole_block(blk_no, (char*)blk_buf);
        if (scale == 1) {
            ret = blk_buf[bid];
            break;
        }
        blk_no = blk_buf[bid / scale];
        bid = bid % scale;
        scale /= _fs.bids_in_blk;
    }

    delete blk_buf;
    return ret;
}

//bid is relative to inode
int Ext2Fs::iread_block(ext2_inode_t* eip, uint32_t bid, off_t off, char* buf, size_t count)
{
    auto rel_id = bid;
    kassert(bid <= inode_occupied_blocks(eip));

    bid = iget_indirect_block_no(eip, bid);
    if (rel_id >= EXT2_NDIR_BLOCKS) kassert (bid != 0);

    uint32_t sect = BLK2SECT(bid) + off / BYTES_PER_SECT;
    uint32_t off_in_sect = off % BYTES_PER_SECT;
    uint32_t end_sect = BLK2SECT(bid) + (off+count-1) / BYTES_PER_SECT;
    uint32_t sz = 0;

    //kprintf("%s: rel %d, abs bid %d, off %d, count %d, sect %d-%d\n", 
            //__func__, rel_id, bid, off, count, sect, end_sect);
    while (sect <= end_sect) {
        Buffer* bufp = bio.read(_dev, sect);
        size_t nr_read = min(count, BYTES_PER_SECT - off_in_sect);
        memcpy(buf + sz, bufp->data + off_in_sect, nr_read);
        sz += nr_read;
        count -= nr_read;
        ++sect;
        off_in_sect = 0;
        bio.release(bufp);
    }
    return sz;
}

/* 
 * In revision 0, (signed) 32bit value indicating the size of the file in 
 * bytes. In revision 1 and later revisions, and only for regular files,
 * this represents the lower 32-bit of the file size; the upper 32-bit is
 * located in the i_dir_acl.
 */
//FIXME: don't support 64bit size now
int Ext2Fs::iread(ext2_inode_t* eip, off_t off, char* buf, size_t count)
{
    if (is_regular(eip)) {
        if (off + count >= eip->size) count = eip->size - off;
    } else if (is_dir(eip)) {
        uint32_t sz = eip->sectors * BYTES_PER_SECT;
        if (off + count >= sz) count = sz - off;
    }
    off_t blk = off / _fs.block_size;
    off_t off_in_blk = off % _fs.block_size;
    off_t end_blk = (off + count - 1) / _fs.block_size;
    //kprintf("%s: blk %d - %d, off %d(in blk %d), sz %d\n", __func__,
            //blk, end_blk, off, off_in_blk, count);

    uint32_t sz = 0;
    while (blk <= end_blk) {
        size_t nr_read = min(count, _fs.block_size-off_in_blk);
        iread_block(eip, blk, off_in_blk, buf, nr_read);
        sz += nr_read;
        count -= nr_read;
        blk++;
        off_in_blk = 0;
    }
    return 0;
}

int Ext2Fs::readdir(File* filp, dentry_t* de, filldir_t) 
{
    inode_t* dir = filp->inode();
    off_t off = filp->off();
    if (dir->type != FsNodeType::Dir) return -EINVAL;
    
    ext2_inode_t* deip = (ext2_inode_t*)dir->data;
    if (off >= deip->size) return 0;
    if (deip->flags & EXT2_INDEX_FL) {
        kprintf("WARN: indexed dir dont supported yet\n");
    }
    
    char* buf = new char[sizeof(ext2_dirent_t)+255];

    auto oflags = ext2lock.lock();
    iread(deip, off, buf, sizeof(ext2_dirent_t)+255);
    ext2lock.release(oflags);
    ext2_dirent_t* dirent = (ext2_dirent_t*)buf;
    
    //kprintf("%s: ino %d, rec_len: %d, namelen: %d\n",
            //__func__, dirent->inode, dirent->rec_len, dirent->name_len);
    //notice that rec_len could be padded to end of block
    if (dirent->inode != 0) {
        de->ip = vfs.alloc_inode(_pdev, dirent->inode);
        kassert(de->ip);
        if (de->ip->ref == 0) {
            de->ip->ref = 1;
            de->ip->ino = dirent->inode;
            read_inode(de->ip);
        }

        filp->set_off(off + dirent->rec_len);
        strncpy(de->name, dirent->name, dirent->name_len);
        delete buf;
        return 1;
    }

    delete buf;
    return 0;
}

void Ext2Fs::read_inode(inode_t* ip)
{
    auto* eip = iget(ip->ino);
    ip->mode = eip->mode;
    ip->dev = _pdev;
    ip->type = FsNodeType::Dir;
    ip->size = isize(eip, &_sb);
    ip->blksize = _fs.block_size;
    ip->blocks = eip->sectors / _fs.sectors_per_block;
    ip->fs = this;
    ip->data = eip;
}

ext2_inode_t* Ext2Fs::iget(uint32_t ino)
{
    int bg_id = (ino - 1) / _sb.inodes_per_group;
    uint32_t ord = (ino - 1) % _sb.inodes_per_group;
    uint32_t sect_off = ord / _fs.inodes_per_sector;
    uint32_t bytes_off = (ord % _fs.inodes_per_sector) * EXT2_INODE_SIZE(_sb);
   
    ext2_inode_t* eip = new ext2_inode_t;
    uint32_t itbl_sector = BLK2SECT(_gdt[bg_id].desc.inode_table);

    //kprintf("%s: bg %d, itbl %d(%d), sectoff %d, bytesoff %d\n",
            //__func__, bg_id, _gdt[bg_id].desc.inode_table, 
            //itbl_sector, sect_off, bytes_off);
    Buffer* bufp = bio.read(_dev, itbl_sector + sect_off);
    *eip = *(ext2_inode_t*)(bufp->data + bytes_off);
    //FIXME: why memcpy failed
    //memcpy((char*)eip, &bufp->data + bytes_off, sizeof *eip);
    bio.release(bufp);

    return eip;
}

FileSystem* create_ext2fs(const void* data)
{
    dev_t dev = (dev_t)data;
    auto* fs = new Ext2Fs;
    fs->init(dev);
    return fs;
}

