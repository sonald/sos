#include "common.h"
#include "isr.h"
#include "vm.h"

#define addr_in_kernel_space(vaddr) ((u32)(vaddr) > KERNEL_VIRTUAL_BASE)

page_directory_t* _kernel_page_directory = NULL;

extern char _data[];

// There is one page table per process, plus one that's used when
// a CPU is not running any process (kpgdir). The kernel uses the
// current process's page table during system calls and interrupts;
// page protection bits prevent user code from using the kernel's
// mappings.
// 
// setupkvm() and exec() set up every page table like this:
//
//   0..KERNEL_VIRTUAL_BASE: user memory (text+data+stack+heap), mapped to
//                phys memory allocated by the kernel
//   KERNEL_VIRTUAL_BASE..KERNEL_VIRTUAL_BASE+EXTMEM: mapped to 0..EXTMEM (for I/O space)
//   KERNEL_VIRTUAL_BASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
//                for the kernel's instructions and r/o data
//   data..KERNEL_VIRTUAL_BASE+PHYSTOP: mapped to V2P(data)..PHYSTOP, 
//                                  rw data + free physical memory
//   0xfe000000..0: mapped direct (devices such as ioapic)
//
// The kernel allocates physical memory for its heap and for user memory
// between V2P(end) and the end of physical memory (PHYSTOP)
// (directly addressable from end..P2V(PHYSTOP)).
static struct {
    void* vaddr;
    u32 phys_start;
    u32 phys_end;
    int perm;
} kmap[] = {
    {(void*)KERNEL_VIRTUAL_BASE, 0, EXTMEM,  PDE_WRITABLE }, // LOW MEM (I/O)
    {(void*)KERNLINK, V2P(KERNLINK), V2P(_data), 0},  // kernel code , RO
    {(void*)_data, V2P(_data), PHYSTOP, PDE_WRITABLE} // kern max usable mem
};

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


static u8 _vmm_base[sizeof(VirtualMemoryManager)];
VirtualMemoryManager* VirtualMemoryManager::_instance = NULL;
VirtualMemoryManager* VirtualMemoryManager::get()
{
    if (!_instance) _instance = new((void*)&_vmm_base) VirtualMemoryManager;
    return _instance;
}

VirtualMemoryManager::VirtualMemoryManager()
    :_pmm(0), _current_pdir(NULL)
{
}

//TODO: set mapping according to mmap from grub
bool VirtualMemoryManager::init(PhysicalMemoryManager* pmm)
{
    _pmm = pmm;

    //kmap is kernel side mapping for VAS, and is big enough for kernel usage,
    //and won't consume more than 4MB memories (which is setupped by 
    //boot_kernel_directory).
    _kernel_page_directory = create_address_space();
    switch_page_directory(_kernel_page_directory);
    kprintf("_kernel_page_directory(0x%x)  ", _kernel_page_directory);
    //dump_page_directory(_kernel_page_directory);

    register_isr_handler(PAGE_FAULT, page_fault);
    return true;
}

void VirtualMemoryManager::dump_page_directory(page_directory_t* pgdir)
{
    if (!pgdir) return;
    kputs("DIRS: \n");
    for (int i = 0; i < 1024; ++i) {
        // show only non-empty 
        u32 pde = pgdir->tables[i];
        if (pde & PDE_PRESENT) {
            page_table_t* ptable = (page_table_t*)p2v(PDE_GET_TABLE_PHYSICAL(pde));
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
        }
    }
    kputchar('\n');
}

void VirtualMemoryManager::switch_page_directory(page_directory_t *pgdir)
{
    if (_current_pdir != pgdir) {
        u32 paddr = v2p(pgdir);
        kprintf("switch pdir at 0x%x\n", paddr);
        asm ("mov %0, %%cr3" :: "r"(paddr));
        _current_pdir = pgdir;
    }
}

void VirtualMemoryManager::flush_tlb_entry(u32 vaddr)
{
    asm volatile ("invlpg (%0)"::"r"(vaddr): "memory");
}

page_t* VirtualMemoryManager::walk(page_directory_t* pgdir, void* vaddr, bool create)
{
    u32 pde = pgdir->tables[PAGE_DIR_IDX(vaddr)];
    page_table_t* ptable = NULL;
    if (pde & PDE_PRESENT) {
        ptable = (page_table_t*)p2v(PDE_GET_TABLE_PHYSICAL(pde));
    } else if (create) {
        ptable = (page_table_t*)alloc_page();
        if (!ptable) panic("walk: no mem");

        memset(ptable, 0, sizeof(page_table_t));

        pde = 0;
        pde_set_flag(pde, PDE_WRITABLE|PDE_USER|PDE_PRESENT);
        pde_set_frame(pde, v2p(ptable));
        pgdir->tables[PAGE_DIR_IDX(vaddr)] = pde;
    } else {
        return NULL;
    }
    return &ptable->pages[PAGE_TABLE_IDX(vaddr)];
}

void VirtualMemoryManager::map_pages(page_directory_t* pgdir, void *vaddr, 
        u32 size, u32 paddr, u32 flags)
{
    char* v = (char*)PGROUNDDOWN(vaddr);
    char* end = (char*)PGROUNDDOWN((u32)vaddr + size - 1);

    kprintf("mapping v(0x%x: 0x%x) -> (0x%x), count: %d\n", v, end, paddr, 
            size / _pmm->frame_size);
    while (v <= end) {
        page_t* pte = walk(pgdir, v, true);
        if (pte == NULL) return;

        if (pte->present) panic("map_pages: remap 0x%x\n", vaddr);
        pte->present = 1;
        pte->rw = ((flags & PDE_WRITABLE) ? 1: 0);
        pte->user = ((flags & PDE_USER) ? 1 : 0);
        pte->frame = paddr / _pmm->frame_size;

        v += _pmm->frame_size;
        paddr += _pmm->frame_size;
    }
}

page_directory_t* VirtualMemoryManager::create_address_space()
{
    page_directory_t* pdir = (page_directory_t*)alloc_page();
    if (!pdir) panic("oom");

    kprintf("create_address_space: pdir = 0x%x(0x%x)\n", pdir, v2p(pdir));
    memset(pdir, 0x0, sizeof(page_directory_t));
    for (int i = 0, len = sizeof(kmap)/sizeof(kmap[0]); i < len; i++) {
        map_pages(pdir, kmap[i].vaddr, kmap[i].phys_end - kmap[i].phys_start, 
                kmap[i].phys_start, kmap[i].perm);
    }

    return pdir;
}

void* VirtualMemoryManager::alloc_page()
{
    u32 paddr = _pmm->alloc_frame();
    return p2v(paddr);
}

void VirtualMemoryManager::free_page(void* vaddr)
{
    kassert(addr_in_kernel_space(vaddr));
    _pmm->free_frame(v2p(vaddr));
}

void* VirtualMemoryManager::kmalloc(int size, int flags)
{
    return NULL;
}

void VirtualMemoryManager::kfree(void* ptr)
{
    //nothing
}

