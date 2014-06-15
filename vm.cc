#include "common.h"
#include "isr.h"
#include "vm.h"

extern page_directory_t _kernel_page_directory;
extern page_table_t table001, table768;

//FIXME: can not use kernel_virtual_base if the current pgdir is not 
//kernel page dir
u32 VirtualMemoryManager::vadd_to_phy(page_directory_t* pgdir, u32 vaddr)
{
    u32 pde = pgdir->tables[PAGE_DIR_IDX(vaddr)];
    kassert(pde & PDE_PRESENT);
    page_table_t* ptable =
        (page_table_t*)(PDE_GET_TABLE_PHYSICAL(pde) + kernel_virtual_base);
    page_t* pte = &ptable->pages[PAGE_TABLE_IDX(vaddr)];
    return pte->frame * _pmm.frame_size;
}

static void page_fault(registers_t* regs)
{
    // read cr2
    u32 fault_addr;
    __asm__ __volatile__ ("movl %%cr2, %0":"=r"(fault_addr));
    int present = !(regs->errcode & 0x1); // Page not present
    int rw = regs->errcode & 0x2;           // Write operation?
    int us = regs->errcode & 0x4;           // Processor was in user-mode?
    int rsvd = regs->errcode & 0x8; // reserved bit in pd set

    kprintf("PF: addr 0x%x, %s, %c, %c, %s\n", fault_addr,
            (present?"P":"NP"), (rw?'W':'R'), (us?'U':'S'), (rsvd?"RSVD":""));
    panic("page_fault");
}

VirtualMemoryManager::VirtualMemoryManager(PhysicalMemoryManager& pmm)
    :_pmm(pmm), _current_pdir(NULL)
{
}

bool VirtualMemoryManager::init()
{
    //trick: bind PDE in boot.s
    kprintf("_kernel_page_directory(0x%x): table1(0x%x), table768(0x%x)\n",
            &_kernel_page_directory, &table001, &table768);

    u32 pde = _kernel_page_directory.tables[768];
    u32 paddr = vadd_to_phy(&_kernel_page_directory, (u32)&table768);
    kassert(PDE_GET_TABLE_PHYSICAL(pde) == paddr);

    //this switch is not necessary and not efficient (invalidates TLB), 
    //just for test, may use _current_pdir = &_kernel_page_directory;
    switch_page_directory(&_kernel_page_directory);
    register_isr_handler(PAGE_FAULT, page_fault);
    return true;
}

void VirtualMemoryManager::dump_page_directory(page_directory_t* pgdir)
{
    if (!pgdir) return;
    kputs("DIRS: \n");
    for (int i = 0; i < 1024; ++i) {
        // show only non-empty 
        if (pgdir->tables[i]) {
            kprintf("%d: 0x%x\t", i, pgdir->tables[i]);
            page_table_t* pde = (page_table_t*)(PDE_GET_TABLE_PHYSICAL(pgdir->tables[i]) + kernel_virtual_base);
            kputs("PAGES: ");
           for (int j = 0; j < 12; ++j) {
               if (pde->pages[j].present) {
                   kprintf("%d: 0x%x\t", j, pde->pages[j]);
               }
           } 
           kputs("\n");
        }
    }
    kputs("\n");
}

bool VirtualMemoryManager::alloc_page(page_t* pte)
{
    void* paddr = _pmm.alloc_frame();
    if (!paddr) return false;

    pte->present = 1;
    pte->frame = (u32)paddr / _pmm.frame_size;
    return true;
}

bool VirtualMemoryManager::free_page(page_t* pte)
{
    void* paddr = (void*)(pte->frame * _pmm.frame_size);
    if (paddr) return false;

    _pmm.free_frame(paddr);
    pte->present = 0;
    return true;
}

void VirtualMemoryManager::switch_page_directory(page_directory_t *pgdir)
{
    if (_current_pdir != pgdir) {
        asm ("mov %0, %%cr3" :: "r"((u32)pgdir - kernel_virtual_base));
        _current_pdir = pgdir;
    }
}

void VirtualMemoryManager::flush_tlb_entry(u32 vaddr)
{
    asm volatile ("invlpg (%0)"::"r"(vaddr): "memory");
}

//FIXME: actually, I can not do this without storing vaddrs of all dynamically
//alloced page tables
page_t* VirtualMemoryManager::lookup_page(u32 vaddr)
{
    panic("wrong implementation");
    page_directory_t* pgdir = current_directory();
    u32 pde = pgdir->tables[PAGE_DIR_IDX(vaddr)];
    page_table_t* ptable =
        (page_table_t*)(PDE_GET_TABLE_PHYSICAL(pde) + kernel_virtual_base);
    page_t* pte = &ptable->pages[PAGE_TABLE_IDX(vaddr)];
}

page_t* VirtualMemoryManager::ptable_lookup_entry(page_table_t* pt, u32 vaddr)
{
    if (pt) return &pt->pages[PAGE_TABLE_IDX(vaddr)];
    return NULL;
}

u32* VirtualMemoryManager::pdirectory_lookup_entry(page_directory_t* pd, u32 vaddr)
{
    if (pd) return &pd->tables[PAGE_DIR_IDX(vaddr)];
    return NULL;
}

void VirtualMemoryManager::map_page(void* paddr, void* vaddr)
{
    if (_current_pdir == NULL) return;

    page_table_t* ptable = NULL;
    u32 pde = _current_pdir->tables[PAGE_DIR_IDX((u32)vaddr)];
    if (!(pde & PDE_PRESENT)) {
        ptable = (page_table_t*)_pmm.alloc_frame();
        if (!ptable) panic("map page failed");
        memset(ptable, 0, sizeof(page_table_t));

        pde_set_flag(pde, PDE_PRESENT);
        pde_set_flag(pde, PDE_WRITABLE);
        pde_set_frame(pde, ptable);
        _current_pdir->tables[PAGE_DIR_IDX((u32)vaddr)] = pde;
    } else {
        ptable = (page_table_t*)PDE_GET_TABLE_PHYSICAL(pde);
    }

    page_t* pte = &ptable->pages[PAGE_TABLE_IDX((u32)vaddr)];
    kassert(pte != NULL);
    kassert(!pte->present);
    pte->present = 1;
    pte->rw = 1;
    pte->frame = (u32)paddr / _pmm.frame_size;
}



