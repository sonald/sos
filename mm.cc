#include "common.h"
#include "mm.h"

#ifdef DEBUG
#define debug_mm(fmt, ...) kprintf("[%s]: " fmt, __func__, ##__VA_ARGS__)
#else
#define debug_mm(fmt, ...)
#endif

static inline u32 aligned(u32 size, u32 frame_size)
{
    return size / frame_size + ((size % frame_size) ? 1 : 0);
}

// PMM
void PhysicalMemoryManager::set_frame(u32 frame_addr)
{
    u32 fid = frame_addr >> 12;
    _frames[fid/32] |= (1<<(fid%32));
}

void PhysicalMemoryManager::clear_frame(u32 frame_addr)
{
    u32 fid = frame_addr >> 12;
    _frames[fid/32] &= ~(1<<(fid%32));
}

int PhysicalMemoryManager::test_frame(u32 frame_addr)
{
    u32 fid = frame_addr >> 12;
    return _frames[fid/32] & (1<<(fid%32));
}

u32 PhysicalMemoryManager::get_first_free_frame()
{
    for (u32 i = 0; i < _frameCount / 32; ++i) {
        if (_frames[i] != 0xffffffff) {
            for (u8 j = 0; j < 32; ++j) {
                if (!(_frames[i] & (1<<j))) {
                    return i*32+j;
                }
            }
        }
    }

    debug_mm("failed\n");
    return invalid;
}

u32 PhysicalMemoryManager::get_first_free_region(u32 size)
{
    if (size == 0) return this->invalid;

    u32 count = aligned(size, this->frame_size);
    if (count == 1) return get_first_free_frame();
    debug_mm("find %d frame\n", count);

    for (u32 i = 0; i < _frameCount / 32; ++i) {
        if (_frames[i] != 0xffffffff) {
            for (u8 j = 0; j < 32; ++j) {
                if (!(_frames[i] & (1<<j))) {
                    u32 start = i*32+j, k = 1;
                    for (; k < count; ++k) {
                        if (test_frame((k+start) * this->frame_size)) {
                            break;
                        }
                    }
                    if (k == count) return start;

                }
            }
        }
    }

    debug_mm("failed\n");
    return this->invalid;
}

void PhysicalMemoryManager::set_region(u32 frame_addr, u32 size)
{
    u32 p = frame_addr, e = p + size;
    while (p < e) {
        set_frame(p);
        p += this->frame_size;
    }
}

void PhysicalMemoryManager::clear_region(u32 frame_addr, u32 size)
{
    u32 p = frame_addr, e = p + size;
    while (p < e) {
        clear_frame(p);
        p += this->frame_size;
    }
}

PhysicalMemoryManager::PhysicalMemoryManager(u32 mem_size, u32 map)
    :_frames((u32*)map), _frameCount(0), _frameUsed(0), _memSize(mem_size)
{
    _frameCount = aligned(_memSize * 1024, this->frame_size);
    //mark all frames starting from 0 to _end plus space used by _frames map
    u32 expand = _frameCount / 8 + map - kernel_virtual_base;
    memset(_frames, 0, _frameCount/8);

    //NOTE: note we setup 768th PDE as present for kernel, which means 
    //the first 4MB physical memory should be marked as alloced.
    alloc_region(this->frame_size*1024);
    debug_mm("mem_size: %dKB, map addr: 0x%x, frames: %d, expand: %d\n",
            mem_size, map, _frameCount, expand);
}

// there is a caveat: NULL is actually ambiguous, it may means the very first
// frame or alloc failure. to keep it safe, we asure that first frame is always
// occupied by kernel, so NULL always means failure.
void* PhysicalMemoryManager::alloc_frame()
{
    debug_mm("_frameUsed: %d\n", _frameUsed);
    if (_frameUsed >= _frameCount) return NULL;
    u32 id = get_first_free_frame();
    if (id == this->invalid) return NULL;
    
    u32 paddr = id * this->frame_size;
    set_frame(paddr);
    _frameUsed++;
    return (void*)paddr;
}

void* PhysicalMemoryManager::alloc_region(u32 size)
{
    if (size == 0) return NULL;

    int nframes = aligned(size, frame_size);
    if (_frameUsed + nframes > _frameCount) {
        debug_mm("nframes %d is too large\n", nframes);
        return NULL;
    }

    u32 id = get_first_free_region(size);
    if (id == this->invalid) return NULL;

    u32 paddr = id * this->frame_size;
    set_region(paddr, size);
    _frameUsed += nframes;

    return (void*)paddr;
}

void PhysicalMemoryManager::free_frame(void* paddr)
{
    clear_frame((u32)paddr);
    _frameUsed--;
}

void PhysicalMemoryManager::free_region(void* paddr, u32 size)
{
    clear_region((u32)paddr, size);
    _frameUsed -= aligned(size, frame_size);
}

