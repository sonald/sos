#ifndef _VM_H
#define _VM_H 

#include "common.h"
#include "mm.h"

typedef struct page_s {
    u32 present    : 1;   // Page present in memory
    u32 rw         : 1;   // Read-only if clear, readwrite if set
    u32 user       : 1;   // Supervisor level only if clear
    u32 accessed   : 1;   // Has the page been accessed since last refresh?
    u32 dirty      : 1;   // Has the page been written to since last refresh?
    u32 unused     : 7;   // Amalgamation of unused and reserved bits
    u32 frame      : 20;  // Frame address (shifted right 12 bits)
} page_t;

typedef struct page_table_s {
    page_t pages[1024];
} page_table_t;

enum PAGE_PDE_FLAGS {
    PDE_PRESENT     = 0x01,      
    PDE_WRITABLE    = 0x02,      
    PDE_USER        = 0x04,      
    PDE_PWT         = 0x08,      
    PDE_PCD         = 0x10,       
    PDE_ACCESSED    = 0x20,       
    PDE_DIRTY       = 0x40,       
    PDE_4MB         = 0x80,       
    PDE_CPU_GLOBAL  = 0x100,      
    PDE_LV4_GLOBAL  = 0x200,      
    PDE_FRAME       = 0xFFFFF000  
};

#define pde_set_flag(pde, flag) ((pde) |= (flag))
#define pde_set_frame(pde, frame) (pde = ((pde) & ~PDE_FRAME) | (u32)frame)

typedef struct page_directory_s {
    u32 tables[1024];
} page_directory_t;

#define PAGE_DIR_IDX(vaddr) ((((u32)vaddr) >> 22) & 0x3ff)
#define PAGE_TABLE_IDX(vaddr) ((((u32)vaddr) >> 12) & 0x3ff)
#define PAGE_ENTRY_OFFSET(vaddr) (((u32)vaddr) & 0x00000fff)

#define PDE_GET_TABLE_PHYSICAL(pde) ((pde & 0xfffff000))
#define PTE_GET_PAGE_PHYSICAL(pte) ((pte & 0xfffff000))

class VirtualMemoryManager 
{
    public:
        static VirtualMemoryManager* get();

        bool init(PhysicalMemoryManager* pmm);

        bool alloc_page(page_t* pte);
        bool free_page(page_t* pte);

        page_t* lookup_page(u32 vaddr);

        page_t* ptable_lookup_entry(page_table_t* pt, u32 vaddr);
        u32* pdirectory_lookup_entry(page_directory_t* p, u32 vaddr);

        // setup a page for vaddr <-> paddr mapping
        void map_page(void* paddr, void* vaddr, u32 flags);


        /**
         * Causes the specified page directory to be loaded into the
         * CR3 register.
         **/
        void switch_page_directory(page_directory_t *pgdir);
        page_directory_t* current_directory() const { return _current_pdir; }
        void flush_tlb_entry(u32 vaddr); 

        void dump_page_directory(page_directory_t* pgdir);
        page_directory_t* create_address_space();

        void* kmalloc(int size, int flags);
        void kfree(void* ptr);

    private:
        static VirtualMemoryManager* _instance;
        PhysicalMemoryManager* _pmm;
        page_directory_t* _current_pdir;
        void* _temp_page_frame_vaddr;
        void* _kernel_heap_start;

        u32 vadd_to_phy(page_directory_t* pgdir, u32 vaddr);
        page_table_t* map_ptable_temporary(u32 table_paddr);
        void unmap_ptable_temporary(u32 table_paddr);

        VirtualMemoryManager();
};

#endif
