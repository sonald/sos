#ifndef _VM_H
#define _VM_H 

#include "mm.h"
#include "memlayout.h"
#include "mmu.h"

class VirtualMemoryManager 
{
    public:
        static VirtualMemoryManager* get();

        bool init(PhysicalMemoryManager* pmm);

        page_t* walk(page_directory_t* pgdir, void* vaddr, bool create = false);
        void map_pages(page_directory_t* pgdir, void *vaddr, u32 size,
                u32 paddr, u32 flags);

        /**
         * Causes the specified page directory to be loaded into the
         * CR3 register.
         **/
        void switch_page_directory(page_directory_t *pgdir);
        page_directory_t* current_directory() const { return _current_pdir; }
        void flush_tlb_entry(u32 vaddr); 

        void dump_page_directory(page_directory_t* pgdir);
        page_directory_t* create_address_space();

        // alloc one kernel page and return vaddr 
        void* alloc_page();
        void free_page(void* vaddr);

        void* kmalloc(int size, int flags);
        void kfree(void* ptr);

    private:
        static VirtualMemoryManager* _instance;
        PhysicalMemoryManager* _pmm;
        page_directory_t* _current_pdir;

        VirtualMemoryManager();
};

#endif
