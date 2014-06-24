#ifndef _MM_H
#define _MM_H 

#include "common.h"

class PhysicalMemoryManager {
    public:
        static constexpr int frame_size = 4096; //4k
        static constexpr int frame_mask = 0xfffff000;
        static constexpr u32 invalid = (u32)-1; // invalid frame number

        PhysicalMemoryManager();
        void init(u32 mem_size);

        u32 memSize() const { return _memSize; }

        /**
         * alloc first free frame and return physical address
         */
        void* alloc_frame();
        void free_frame(void* paddr);
        /**
         * alloc first consective size/frame_size of free frames and 
         * return physical address
         */
        void* alloc_region(u32 size);
        void free_region(void* paddr, u32 size);
        
    private:
        void set_frame(u32 frame_addr);
        void clear_frame(u32 frame_addr);
        void set_region(u32 frame_addr, u32 size);
        void clear_region(u32 frame_addr, u32 size);
        int test_frame(u32 frame_addr);
        u32 get_first_free_frame();
        u32 get_first_free_region(u32 size);

    private:
        u32* _frames;
        u32 _frameCount;
        u32 _frameUsed;
        u32 _memSize; // in KB
};


#endif
