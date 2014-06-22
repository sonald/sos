#include "common.h"
#include "isr.h"
#include "vm.h"

#define addr_in_kernel_space(vaddr) (((u32)vaddr) > kernel_virtual_base)

extern page_directory_t _kernel_page_directory;
extern page_table_t table001, table768;

//FIXME: can not use kernel_virtual_base if the current pgdir is not 
//kernel page dir
u32 VirtualMemoryManager::vadd_to_phy(page_directory_t* pgdir, u32 vaddr)
{
    if (addr_in_kernel_space(vaddr)) {
        u32 pde = pgdir->tables[PAGE_DIR_IDX(vaddr)];
        kassert(pde & PDE_PRESENT);
        page_table_t* ptable =
            (page_table_t*)(PDE_GET_TABLE_PHYSICAL(pde) + kernel_virtual_base);
        page_t* pte = &ptable->pages[PAGE_TABLE_IDX(vaddr)];
        return pte->frame * _pmm.frame_size;
    }

    panic("can not determine physical of none kernel space address");
    return 0;
}

static void page_fault(registers_t* regs)
{
    // read cr2
    u32 fault_addr;
    kprintf("regs: ds: 0x%x, edi: 0x%x, esi: 0x%x, ebp: 0x%x, esp: 0x%x\n"
            "ebx: 0x%x, edx: 0x%x, ecx: 0x%x, eax: 0x%x, isr: %d, errno: %d\n"
            "eip: 0x%x, cs: 0x%x, eflags: 0b%b, useresp: 0x%x, ss: 0x%x\n",
            regs->ds, regs->edi, regs->esi, regs->ebp, regs->esp, 
            regs->ebx, regs->edx, regs->ecx, regs->eax, regs->isrno, regs->errcode,
            regs->eip, regs->cs, regs->eflags, regs->useresp, regs->ss);

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

//TODO: set mapping according to mmap from grub
bool VirtualMemoryManager::init()
{
    //trick: bind PDE in boot.s
    kprintf("_kernel_page_directory(0x%x): table1(0x%x), table768(0x%x)\n",
            &_kernel_page_directory, &table001, &table768);

    u32 pde = _kernel_page_directory.tables[768];
    u32 paddr = vadd_to_phy(&_kernel_page_directory, (u32)&table768);
    kassert(PDE_GET_TABLE_PHYSICAL(pde) == paddr);

    for (int i = 769; i < 1024; ++i) {
        u32* pde = &_kernel_page_directory.tables[i];
        pde_set_flag(*pde, (PDE_PRESENT|PDE_WRITABLE));
        paddr += _pmm.frame_size;
        pde_set_frame(*pde, paddr);
        //if ( i < 800)
            //kprintf("pdir(%d) -> 0x%x  ", i, paddr);
    }

    //see map_ptable_temporary
    _kernel_tables_start = (void*)(0xFFFFF000);

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
    for (int i = 768; i < 1024; ++i) {
        // show only non-empty 
        u32 pde = pgdir->tables[i];
        if (pde & PDE_PRESENT) {
            page_table_t* ptable = map_ptable_temporary(PDE_GET_TABLE_PHYSICAL(pde));
            int count = 0;
            for (int j = 0; j < 1024; ++j) {
                if (ptable->pages[j].present) {
                    count++;
                }
            } 

            if (count > 0) {
                kprintf("%d: 0x%x, ", i, pgdir->tables[i]);
                kprintf("%d PAGES\t", count);
            }

            unmap_ptable_temporary((u32)ptable);
        }
    }
    kputchar('\n');
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
//alloced page tables. in order for it to work properly, some rules must be
//setuped. i.e all virtual address up from KERNEL_VIRTUAL_BASE (kernel virtual 
//space) in all page dirs has the same mapping rule.
page_t* VirtualMemoryManager::lookup_page(u32 vaddr)
{
    //panic("wrong implementation");
    page_directory_t* pgdir = current_directory();
    u32 pde = pgdir->tables[PAGE_DIR_IDX(vaddr)];
    page_table_t* ptable =
        (page_table_t*)(PDE_GET_TABLE_PHYSICAL(pde) + kernel_virtual_base);
    page_t* pte = &ptable->pages[PAGE_TABLE_IDX(vaddr)];
    return pte;
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

void* VirtualMemoryManager::alloc_vaddr_for_user_table()
{
    return _kernel_tables_start;
}

page_table_t* VirtualMemoryManager::map_ptable_temporary(u32 table_paddr)
{
    page_directory_t* pgdir = _current_pdir;

    void* user_table_frame = (void*)table_paddr;

    void* table_vaddr = NULL;
    page_table_t* pde_ptable = NULL;

    //map table's page frame into kernel virtual space, so 
    //we can access and modify it.
    table_vaddr = alloc_vaddr_for_user_table();
    u32 pde = pgdir->tables[PAGE_DIR_IDX(table_vaddr)];
    kassert(pde & PDE_PRESENT);
    pde_ptable =
        (page_table_t*)(PDE_GET_TABLE_PHYSICAL(pde) + kernel_virtual_base);
    page_t* pte = &pde_ptable->pages[PAGE_TABLE_IDX(table_vaddr)];
    pte->rw = 1;
    pte->present = 1;
    pte->frame = (u32)user_table_frame / _pmm.frame_size;
    flush_tlb_entry((u32)table_vaddr);

    return (page_table_t*)table_vaddr;
}

void VirtualMemoryManager::unmap_ptable_temporary(u32 table_vaddr)
{
    u32 pde = _current_pdir->tables[PAGE_DIR_IDX(table_vaddr)];
    kassert(pde & PDE_PRESENT);
    page_table_t* pde_ptable =
        (page_table_t*)(PDE_GET_TABLE_PHYSICAL(pde) + kernel_virtual_base);
    page_t* pte = &pde_ptable->pages[PAGE_TABLE_IDX(table_vaddr)];
    // remove temporary mapping
    *(u32*)pte = 0x0;
    flush_tlb_entry((u32)table_vaddr);
}

void VirtualMemoryManager::map_page(void* paddr, void* vaddr, u32 flags)
{
    if (vaddr >= _kernel_tables_start) {
        panic("reserved page, cannot mapped\n");
        return;
    }

    if (addr_in_kernel_space(vaddr)) {
        u32 pde = _current_pdir->tables[PAGE_DIR_IDX(vaddr)];
        kassert (pde & PDE_PRESENT);
        page_table_t* ptable = 
            (page_table_t*)(PDE_GET_TABLE_PHYSICAL(pde) + kernel_virtual_base);

        page_t* pte = &ptable->pages[PAGE_TABLE_IDX(vaddr)];
        pte->rw = (flags & 0x1 ? 1: 0);
        pte->present = 1;
        pte->user = (flags & 0x4 ? 1 : 0);
        pte->frame = (u32)paddr / _pmm.frame_size;
        return;
    }

    kprintf("map_page: vaddr dir %d, tid: %d\n", PAGE_DIR_IDX(vaddr),
            PAGE_TABLE_IDX(vaddr));

    page_directory_t* pgdir = _current_pdir;
    void* user_table_frame = NULL;

    u32 user_pde = pgdir->tables[PAGE_DIR_IDX(vaddr)];
    if (user_pde & PDE_PRESENT) {
        user_table_frame = (void*)(PDE_GET_TABLE_PHYSICAL(user_pde));
    } else {
        user_table_frame = _pmm.alloc_frame();
        if (!user_table_frame) panic("no free memory");

        pde_set_flag(user_pde, flags|PDE_PRESENT);
        pde_set_frame(user_pde, user_table_frame);
        pgdir->tables[PAGE_DIR_IDX(vaddr)] = user_pde;
    }

    page_table_t* user_ptable = map_ptable_temporary((u32)user_table_frame);
    if (!(user_pde & PDE_PRESENT)) {
        memset(user_ptable, 0x0, _pmm.frame_size);
    }

    page_t* user_pte = &user_ptable->pages[PAGE_TABLE_IDX(vaddr)];
    user_pte->rw = (flags & 0x1 ? 1: 0);
    user_pte->present = 1;
    user_pte->user = (flags & 0x4 ? 1 : 0);
    user_pte->frame = (u32)paddr / _pmm.frame_size;

    unmap_ptable_temporary((u32)user_ptable);
}

