#include "vfs.h"

bool Initramfs::load()
{
}

u32 Initramfs::read(inode_t* node, u32 offset, u32 size, u8* buffer)
{
}

u32 Initramfs::write(inode_t* node, u32 offset, u32 size, u8* buffer)
{
}

void Initramfs::open(inode_t* node, u8 read, u8 write)
{
}

void Initramfs::close(inode_t* node)
{
}

struct dirent* Initramfs::readdir(inode_t* node, u32 index)
{
}

inode_t* Initramfs::finddir(inode_t* node, char* name)
{
}
