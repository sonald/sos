#include "common.h"
#include "mm.h"
#include "mmu.h"

#ifdef DEBUG
#define debug_mm(fmt, ...) kprintf("[%s]: " fmt, __func__, ##__VA_ARGS__)
#else
#define debug_mm(fmt, ...)
#endif

extern u32* _end;

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

u32 PhysicalMemoryManager::get_last_free_frame()
{
    for (int i = _frameCount/32-1; i >= 0; --i) {
        if (_frames[i] != 0xffffffff) {
            for (int j = 31; j >= 0; --j) {
                if (!(_frames[i] & (1<<j))) {
                    return i*32+j;
                }
            }
        }
    }

    return invalid;
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

    return invalid;
}

u32 PhysicalMemoryManager::get_first_free_region(u32 size)
{
    if (size == 0) return this->invalid;

    u32 count = aligned(size, this->frame_size);
    if (count == 1) return get_first_free_frame();
    //debug_mm("find %d frame\n", count);

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

PhysicalMemoryManager::PhysicalMemoryManager()
    :_frames((u32*)&_end), _frameCount(0), _frameUsed(0)
{
    kputs("new PhysicalMemoryManager\n");
}

void PhysicalMemoryManager::init(u32 mem_size, void* last_used)
{
    _memSize = mem_size;
    if (last_used) {
        _frames = (u32*)last_used;
    }
    _frameCount = aligned(_memSize * 1024, this->frame_size);
    memset(_frames, 0, _frameCount/8);

    // from there, we can alloc memory for kernel
    _freeStart = (u32*)PGROUNDUP(A2I(_frames) + _frameCount/32);

    u32 used_mem = PGROUNDUP(v2p(_freeStart)), // in Bytes
        free_mem = mem_size * 1024 - used_mem; // in Bytes

    if (free_mem < PHYSTOP) {
        _freeEnd = _freeStart + free_mem/4;
    } else {
        _freeEnd = _freeStart + PHYSTOP/4;
    }

    alloc_region(used_mem);

    debug_mm("mem_size: %dKB, used: %dKB, pmap addr: 0x%x, frames: %d,"
            " used: %d, freestart: 0x%x, freeend: 0x%x\n",
            mem_size, used_mem/1024, _frames, _frameCount, 
            _frameUsed, _freeStart, _freeEnd);
}

// there is a caveat: NULL is actually ambiguous, it may means the very first
// frame or alloc failure. to keep it safe, we asure that first frame is always
// occupied by kernel, so NULL always means failure.
u32 PhysicalMemoryManager::alloc_frame()
{
    //debug_mm("_frameUsed: %d\n", _frameUsed);
    if (_frameUsed >= _frameCount) return 0;
    u32 id = get_first_free_frame();
    if (id == this->invalid) return 0;
    
    u32 paddr = id * this->frame_size;
    set_frame(paddr);
    _frameUsed++;
    return paddr;
}

u32 PhysicalMemoryManager::alloc_frame_tail()
{
    //debug_mm("_frameUsed: %d\n", _frameUsed);
    if (_frameUsed >= _frameCount) return 0;
    u32 id = get_last_free_frame();
    if (id == this->invalid) return 0;
    
    u32 paddr = id * this->frame_size;
    set_frame(paddr);
    _frameUsed++;
    return paddr;
}

u32 PhysicalMemoryManager::alloc_region(u32 size)
{
    if (size == 0) return 0;

    int nframes = aligned(size, frame_size);
    if (_frameUsed + nframes > _frameCount) {
        debug_mm("nframes %d is too large\n", nframes);
        return 0;
    }

    u32 id = get_first_free_region(size);
    if (id == this->invalid) {
        debug_mm("get_first_free_region failed\n");
        return 0;
    }

    u32 paddr = id * this->frame_size;
    set_region(paddr, size);
    _frameUsed += nframes;

    return paddr;
}

void PhysicalMemoryManager::free_frame(u32 paddr)
{
    clear_frame(paddr);
    _frameUsed--;
}

void PhysicalMemoryManager::free_region(u32 paddr, u32 size)
{
    clear_region(paddr, size);
    _frameUsed -= aligned(size, frame_size);
}

