#ifndef _MM_H
#define _MM_H 

#include "mmu.h"
#include "memlayout.h"

class PhysicalMemoryManager {
    public:
        friend class VirtualMemoryManager;

        static constexpr int frame_size = PGSIZE; //4k
        static constexpr u32 invalid = (u32)-1; // invalid frame number

        PhysicalMemoryManager();
        void init(u32 mem_size);

        u32 mem_size() const { return _memSize; }

        /**
         * alloc first free frame and return physical address
         */
        u32 alloc_frame();
        // alloc first free frame from the end of memory
        u32 alloc_frame_tail();
        void free_frame(u32 paddr);

        /**
         * alloc first consective size/frame_size of free frames and 
         * return physical address
         */
        u32 alloc_region(u32 size);
        void free_region(u32 paddr, u32 size);
        
    private:
        void set_frame(u32 frame_addr);
        void clear_frame(u32 frame_addr);
        void set_region(u32 frame_addr, u32 size);
        void clear_region(u32 frame_addr, u32 size);
        int test_frame(u32 frame_addr);

        u32 get_first_free_frame();
        u32 get_last_free_frame();
        u32 get_first_free_region(u32 size);

    private:
        u32* _frames;
        u32* _framesEnd;
        u32 _frameCount;
        u32 _frameUsed;
        u32 _memSize; // in KB

        u32* _freeStart;
        u32* _freeEnd;
};


#endif
