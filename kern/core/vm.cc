#include <common.h>
#include <string.h>
#include <isr.h>
#include <vm.h>
#include <task.h>

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
//                  phys memory allocated by the kernel
//   KERNEL_VIRTUAL_BASE..KERNEL_VIRTUAL_BASE+EXTMEM:
//                  mapped to 0..EXTMEM (for I/O space)
//   KERNEL_VIRTUAL_BASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
//                  for the kernel's instructions and r/o data
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

static void page_fault(trapframe_t* regs)
{
    // read cr2
    u32 fault_addr;

    __asm__ __volatile__ ("movl %%cr2, %0":"=r"(fault_addr));
    int present = !(regs->errcode & 0x1); // Page not present
    int rw = regs->errcode & 0x2;           // Write operation?
    int us = regs->errcode & 0x4;           // Processor was in user-mode?
    int rsvd = regs->errcode & 0x8; // reserved bit in pd set

    kprintf("regs: ds: 0x%x, edi: 0x%x, esi: 0x%x, ebp: 0x%x, esp: 0x%x\n"
            "eip: 0x%x, cs: 0x%x, eflags: 0b%b, useresp: 0x%x, ss: 0x%x\n",
            regs->ds, regs->edi, regs->esi, regs->ebp, regs->esp,
            regs->eip, regs->cs, regs->eflags, regs->useresp, regs->ss);

    kprintf("PF: addr 0x%x, %s, %c, %c, %s\n", fault_addr,
            (present?"P":"NP"), (rw?'W':'R'), (us?'U':'S'), (rsvd?"RSVD":""));
    if (current) {
        kprintf("current: %s, regs: 0x%x\n", current->name, current->regs);
    }
    panic("");
}

VirtualMemoryManager vmm;

VirtualMemoryManager::VirtualMemoryManager()
    :_pmm(0), _current_pdir(NULL)
{
}

#define KHEAD_SIZE  20  // ignore data field
#define ALIGN(sz, align) ((((u32)(sz)-1)/align)*align+align)

void VirtualMemoryManager::test_malloc()
{
    //set_text_color(COLOR(LIGHT_GREEN, BLACK));
    void* p1 = kmalloc(0x80-3, 4);
    void* p2 = kmalloc(0x100, 8);
    void* p3 = kmalloc(0x400, PGSIZE/2);
    kprintf("p1 = 0x%x, p2 = 0x%x, p3 = 0x%x\n", p1, p2, p3);

    void* p4 = kmalloc(PGSIZE-1, PGSIZE);
    void* p5 = kmalloc(PGSIZE);
    kprintf("p4 = 0x%x\n", p4);
    kfree(p3);
    kfree(p2);
    kfree(p4);
    kfree(p1);

    p1 = kmalloc(PGSIZE*3, PGSIZE);
    kfree(p5);
    kfree(p1);

    dump_heap();
    for(;;);
}

//TODO: set mapping according to mmap from grub
bool VirtualMemoryManager::init(PhysicalMemoryManager* pmm)
{
    _pmm = pmm;

    _kern_heap_limit = _pmm->_freeStart;
    _kheap_ptr = (kheap_block_head*)ksbrk(PGSIZE);


    //kmap is kernel side mapping for VAS, and is big enough for kernel usage,
    //and won't consume more than 4MB memories (which is setupped by
    //boot_kernel_directory).
    _kernel_page_directory = create_address_space();
    switch_page_directory(_kernel_page_directory);

    register_isr_handler(PAGE_FAULT, page_fault);
    //test_malloc();
    return true;
}

page_directory_t* VirtualMemoryManager::kernel_page_directory()
{
    return _kernel_page_directory;
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
        //kprintf("switch pdir at 0x%x\n", paddr);
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
    auto eflags = _lock.lock();
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
        _lock.release(eflags);
        return NULL;
    }
    _lock.release(eflags);
    return &ptable->pages[PAGE_TABLE_IDX(vaddr)];
}

bool VirtualMemoryManager::mapped(page_directory_t* pgdir, void* vaddr)
{
    char* v = (char*)PGROUNDDOWN(vaddr);
    auto eflags = _lock.lock();
    auto* pte = walk(pgdir, v, false);
    _lock.release(eflags);

    return pte && pte->present;
}

void VirtualMemoryManager::map_pages(page_directory_t* pgdir, void *vaddr,
        u32 size, u32 paddr, u32 flags)
{
    char* v = (char*)PGROUNDDOWN(vaddr);
    char* end = (char*)PGROUNDDOWN((u32)vaddr + size - 1);

    auto eflags = _lock.lock();
    //kprintf("mapping v(0x%x: 0x%x) -> (0x%x), count: %d\n", v, end, paddr,
            //size / _pmm->frame_size);
    while (v <= end) {
        page_t* pte = walk(pgdir, v, true);
        if (pte == NULL) break;

        if (pte->present) panic("map_pages: remap 0x%x\n", vaddr);
        pte->present = 1;
        pte->rw = ((flags & PDE_WRITABLE) ? 1: 0);
        pte->user = ((flags & PDE_USER) ? 1 : 0);
        pte->frame = paddr / _pmm->frame_size;

        v += _pmm->frame_size;
        paddr += _pmm->frame_size;
    }
    _lock.release(eflags);
}

void VirtualMemoryManager::unmap_pages(page_directory_t* pgdir, void *vaddr,
        u32 size, bool free_mem)
{
    char* v = (char*)PGROUNDDOWN(vaddr);
    char* end = (char*)PGROUNDDOWN((u32)vaddr + size - 1);

    auto eflags = _lock.lock();
    while (v <= end) {
        page_t* pte = walk(pgdir, v, false);
        kassert(pte != NULL);
        if (!pte->present) panic("%s: 0x%x should be present\n", __func__, vaddr);

        if (free_mem) {
            free_page(p2v(pte->frame * _pmm->frame_size));
        }
        *(u32*)pte = 0;
        flush_tlb_entry((u32)v);

        v += _pmm->frame_size;
    }
    _lock.release(eflags);
}

page_directory_t* VirtualMemoryManager::create_address_space()
{
    page_directory_t* pdir = (page_directory_t*)alloc_page();
    if (!pdir) panic("oom");

    auto eflags = _lock.lock();
    kprintf("create_address_space: pdir = 0x%x(0x%x)\n", pdir, v2p(pdir));
    memset(pdir, 0x0, sizeof(page_directory_t));
    for (int i = 0, len = sizeof(kmap)/sizeof(kmap[0]); i < len; i++) {
        map_pages(pdir, kmap[i].vaddr, kmap[i].phys_end - kmap[i].phys_start,
                kmap[i].phys_start, kmap[i].perm);
    }

    if (_kernel_page_directory) {
        // copy other dynamically created kernel mapping
        // currently video memory mapping
        char* v = (char*)0xE0000000;
        char* end = (char*)0xF0000000;
        while (v < end) {
            page_t* pte = walk(_kernel_page_directory, v, false);
            if (pte && pte->present) {
                u32 paddr = pte->frame * _pmm->frame_size;
                int flags = pde_get_flags(*(u32*)pte);

                map_pages(pdir, v, PGSIZE, paddr, flags);
            }

            v += PGSIZE;
        }
    }
    _lock.release(eflags);
    return pdir;
}

/**
 * used by execv: copy of user-part address space for text and data.
 * semantic: deep copy code, data and user stack now. shallow copy is difficult right now
 */
page_directory_t* VirtualMemoryManager::copy_page_directory(page_directory_t* pgdir)
{
    auto* new_pgdir = create_address_space();
    char* v = (char*)UCODE;
    char* end = (char*)USTACK_TOP;

    auto eflags = _lock.lock();
    while (v < end) {
        page_t* pte = walk(pgdir, v, false);
        if (pte && pte->present) {
            u32 paddr = pte->frame * _pmm->frame_size;
            int flags = pde_get_flags(*(u32*)pte);

            void* new_pg = vmm.alloc_page();
            map_pages(new_pgdir, v, PGSIZE, v2p(new_pg), flags);
            memcpy(new_pg, p2v(paddr), PGSIZE);
        }

        v += PGSIZE;
    }

    _lock.release(eflags);
    return new_pgdir;
}

void VirtualMemoryManager::release_address_space(page_directory_t* pgdir)
{
    auto eflags = _lock.lock();
    // shallow copies of kernel part do not release
    for (int i = 0, len = sizeof(kmap)/sizeof(kmap[0]); i < len; i++) {
        unmap_pages(pgdir, kmap[i].vaddr, kmap[i].phys_end - kmap[i].phys_start, false);
    }

    char* v = (char*)0xE0000000;
    char* end = (char*)0xF0000000;
    while (v < end) {
        page_t* pte = walk(pgdir, v, false);
        if (pte && pte->present) {
            unmap_pages(pgdir, v, PGSIZE, false);
        }
        v += PGSIZE;
    }

    //release user space maps
    //FIXME: what about fored and not execed child. these are shallow copies
    v = (char*)UCODE;
    end = (char*)USTACK_TOP;
    while (v < end) {
        page_t* pte = walk(pgdir, v, false);
        if (pte && pte->present) {
            unmap_pages(pgdir, v, PGSIZE, true);
        }
        v += PGSIZE;
    }

    for (int i = 0; i < 1024; i++) {
        auto pde = pgdir->tables[i];
        if (pde & PDE_PRESENT) {
            auto* ptable = (page_table_t*)p2v(PDE_GET_TABLE_PHYSICAL(pde));
            free_page(ptable);
        }
    }

    free_page(pgdir);
    _lock.release(eflags);
}

void* VirtualMemoryManager::alloc_page()
{
    return kmalloc(PGSIZE, PGSIZE);
}

void VirtualMemoryManager::free_page(void* vaddr)
{
    kfree(vaddr);
}

void* VirtualMemoryManager::ksbrk(size_t size)
{
    kassert(_kern_heap_limit < _pmm->_freeEnd);
    kassert(aligned((void*)size, PGSIZE));

    u32 paddr = _pmm->alloc_region(size);
    kassert(paddr != 0);
    void* ptr = p2v(paddr);

    kheap_block_head* newh = (kheap_block_head*)ptr;
    newh->used = 0;
    newh->next = newh->prev = NULL;
    newh->size = size - KHEAD_SIZE;
    newh->ptr = newh->data;

    //kprintf("[VMM]: ksbrk 0x%x: size 0x%x, ptr 0x%x\n", ptr, size, newh->ptr);
    _kern_heap_limit += size/4;
    return ptr;
}

void* VirtualMemoryManager::kmalloc(size_t size, int align)
{
    size_t realsize = ALIGN(size, align);
    //kprintf("kmalloc: size = %d, realsize = %d, align = %d\n", size, realsize, align);

    auto eflags = _lock.lock();
    kheap_block_head* last = NULL;
    kheap_block_head* h = find_block(&last, realsize);
    if (!h) {
        h = (kheap_block_head*)ksbrk(PGROUNDUP(realsize+KHEAD_SIZE));
        if (!h) panic("[VMM] kmalloc: no free memory\n");

        kassert(last->next == NULL);
        last->next = h;
        h->prev = last;

        if (h->prev && h->prev->used == 0) {
            h = merge_block(h->prev);
        }
    }

    if (!aligned(h->data, align)) {
        h = align_block(h, align);
    }

    if (h->size - realsize >= KHEAD_SIZE + 4) {
        split_block(h, realsize);
    }

    h->used = 1;
    kassert(aligned(h->data, align));
    _lock.release(eflags);
    return h->data;
}

VirtualMemoryManager::kheap_block_head* VirtualMemoryManager::align_block(
        VirtualMemoryManager::kheap_block_head* h, int align)
{
    auto newh = (kheap_block_head*)(ALIGN(&h->data, align)-KHEAD_SIZE);
    int gap = (char*)newh - (char*)h;
    //kprintf("aligned to 0x%x, gap = %d\n", newh, gap);
    if (gap >= KHEAD_SIZE+4) {
        split_block(h, gap-KHEAD_SIZE);
        h = h->next;
        kassert(newh == h);

    } else {
        kassert(gap);
        //drop little memory, which cause frag that never can be regain
        if (h == _kheap_ptr) _kheap_ptr = newh;
        else if (h->prev) {
            auto prev = h->prev;
            prev->size += gap;
            prev->next = newh;
        }
        if (h->next) h->next->prev = newh;
        // h and newh may overlay, so copy aside
        kheap_block_head tmp = *h;
        *newh = tmp;
        newh->size -= gap;
        newh->ptr = newh->data;
    }
    return newh;
}

void VirtualMemoryManager::split_block(VirtualMemoryManager::kheap_block_head* h,
        size_t size)
{
    kheap_block_head *b = (kheap_block_head*)(h->data + size);
    b->used = 0;
    b->size = h->size - size - KHEAD_SIZE;
    b->ptr = b->data;

    b->next = h->next;
    h->next = b;
    b->prev = h;

    if (b->next) b->next->prev = b;
    h->size = size;
}

void VirtualMemoryManager::kfree(void* ptr)
{
    auto eflags = _lock.lock();
    // kprintf("[VMM] kfree 0x%x\n", ptr);
    kheap_block_head* h = (kheap_block_head*)((char*)ptr - KHEAD_SIZE);
    kassert(h->used);
    kassert(h->ptr == h->data);

    h->used = 0;
    if (h->prev && h->prev->used == 0) {
        h = merge_block(h->prev);
    }

    if (h->next) {
        h = merge_block(h);
    }
    _lock.release(eflags);
}

VirtualMemoryManager::kheap_block_head* VirtualMemoryManager::merge_block(
        VirtualMemoryManager::kheap_block_head* h)
{
    kassert(h->next);
    if (h->next->used) return h;

    auto next = h->next;
    //kprintf("merge 0x%x with 0x%x\n", h, h->next);
    h->size += KHEAD_SIZE + next->size;
    h->next = next->next;
    if (h->next) h->next->prev = h;
    return h;
}

VirtualMemoryManager::kheap_block_head* VirtualMemoryManager::find_block(
        VirtualMemoryManager::kheap_block_head** last, size_t size)
{
    kheap_block_head* p = _kheap_ptr;
    while (p && !(p->used == 0 && p->size >= size)) {
        *last = p;
        p = p->next;
    }

    return p;
}

bool VirtualMemoryManager::aligned(void* ptr, int align)
{
    return ((u32)ptr & (align-1)) == 0;
}

void VirtualMemoryManager::dump_heap(int limit)
{
    kprintf("HEAP: ");
    int n = 0;
    kheap_block_head* h = _kheap_ptr;
    while (h) { n++; h = h->next; }
    if (limit == -1) limit = n;
    if (limit > n) limit = n;

    int start = n - limit;
    n = 0;
    h = _kheap_ptr;
    while (h) {
        if (n++ >= start) {
            kprintf("[@0x%x(U: %d): S: 0x%x, N: 0x%x, P: 0x%x]\n",
                    h, h->used, h->size, h->next, h->prev);
            kassert(h->next != _kheap_ptr);
        }
        h = h->next;
    }
    kputchar('\n');
}

