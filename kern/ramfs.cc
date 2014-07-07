#include "vfs.h"
#include "ramfs.h"
#include "vm.h"
#include "common.h"
#include "errno.h"

#define NR_INITRD_FILES 64
#define RAMFS_ENTRY_MAGIC 0xBF
#define NAMELEN 64

typedef struct initrd_entry_header
{
   unsigned char magic; // The magic number is there to check for consistency.
   char name[NAMELEN];
   unsigned int offset; // Offset in the initrd the file starts.
   unsigned int length; // Length of the file.
} __attribute__((packed)) initrd_entry_header_t;

typedef struct initrd_header
{
    u32 nfiles; 
    initrd_entry_header_t files[NR_INITRD_FILES];
} initrd_header_t;

void Ramfs::init(char* addr, size_t size, const char* cmdline)
{
    (void)size;

    int cmd_len = strlen(cmdline);
    _cmdline = (char*)vmm.kmalloc(cmd_len+1, 1);
    strcpy(_cmdline, cmdline);

    _sb = (initrd_header_t*)addr;
    kprintf("files: %d\n", _sb->nfiles);

    u32 dev = DEVNO(RAMFS_MAJOR, 0);
    _iroot = (inode_t*)vmm.kmalloc(sizeof(inode_t), 1);
    _iroot->dev = dev;
    _iroot->type = I_DIR;
    _iroot->ino = 0;

    _nr_nodes = _sb->nfiles;
    _nodes = (inode_t*)vmm.kmalloc(sizeof(inode_t)*_nr_nodes, 1);

    for (int i = 0; i < _nr_nodes; ++i) {
        initrd_entry_header* de = &_sb->files[i];
        kprintf("%s: magic: 0x%x, len: %d, off: %d\n", de->name, 
                de->magic, de->length, de->offset);
        kassert(de->magic == RAMFS_ENTRY_MAGIC);
        _nodes[i].size = de->length;
        _nodes[i].dev = dev;
        _nodes[i].type = I_FILE;
        _nodes[i].ino = i+1;
    }

    kassert(devices[RAMFS_MAJOR] == NULL);
    devices[RAMFS_MAJOR] = this; 
}

int Ramfs::read(inode_t* ip, void* buf, size_t nbyte, u32 offset) 
{
    kassert(ip->type == I_FILE);
    initrd_entry_header* ieh = &_sb->files[ip->ino-1];

    if (offset >= ip->size) return 0;
    if (nbyte + offset >= ip->size) nbyte = ip->size - offset;

    char* data = (char*)_sb + ieh->offset + offset;
    memcpy(buf, data, nbyte);

    return nbyte;
}

int Ramfs::write(inode_t* ip, void* buf, size_t nbyte, u32 offset) 
{
    kassert(ip->type == I_FILE);
    (void)buf;
    (void)nbyte;
    (void)offset;
    return -EINVAL;
}

inode_t* Ramfs::dir_lookup(inode_t* ip, const char* name) 
{
    if (ip->type != I_DIR) return NULL;
    
    int i = -1;
    for (i = 0; i < _nr_nodes; ++i) {
        initrd_entry_header* de = &_sb->files[i];
        if (strncmp(de->name, name, NAMELEN) == 0) {
            break;
        }
    }

    if (i >= _nr_nodes) return NULL;
    return &_nodes[i];
}

dentry_t* Ramfs::dir_read(inode_t* ip, int id) 
{
    if (ip->type != I_DIR) return NULL;
    if (id >= _nr_nodes) return NULL;

    initrd_entry_header_t* ieh = &_sb->files[id];
    dentry_t* de = (dentry_t*)vmm.kmalloc(sizeof(dentry_t), 1);
    strcpy(de->name, ieh->name);
    de->ino = _nodes[id].ino;
    return  de;
}

