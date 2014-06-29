#include "common.h"
#include "boot.h"
#include "gdt.h"
#include "isr.h"
#include "timer.h"
#include "vm.h"
#include "kb.h"
#include "task.h"
#include "syscall.h"

extern "C" int is_cpuid_capable();

/*
 * gets called before Global constructor 
 */
extern "C" void kernel_init()
{
    clear();
    set_text_color(LIGHT_CYAN, BLACK);
}

static inline bool is_support_4m_page()
{
    u32 edx;
    __asm__ (
        "mov %%eax, $1 \n"
        "cpuid \n"
        :"=r"(edx)
        :: "eax"
    );
    return (edx & 0x00000004) == 1;
}

static void test_irqs()
{
    //DIV 0
    int a = 0;
    kprintf("%x\n", 100/a);
    //Page Fault
    int b = *(u32*)(0x01000000);
    kprintf("%x\n", b);
}

static void check_paging()
{
    // check if paging is working correctly
    for (u32 i = 0; i < 1024; ++i) {
        kprintf("%x", *(u32*)(KERNEL_VIRTUAL_BASE + i*4096 + 4));
    }
}

static void test_pmm(PhysicalMemoryManager& pmm)
{
    u32 p = pmm.alloc_frame();
    if (p == 0) kputs("alloc failed\n");
    kputs("alloc finish  ");
    pmm.free_frame(p);
    u32 p2 = pmm.alloc_frame();
    if (p != p2) kputs("p != p2\n");

    u32 size = pmm.frame_size * 10;
    p = pmm.alloc_region(size);
    if (p == 0) kputs("alloc region failed\n");
    pmm.free_region(p, size);
    p2 = pmm.alloc_frame();
    if (p != p2) kputs("p != p2\n");
}

extern "C" void switch_to_usermode(void* ring3_esp, void* ring3_eip);

tss_entry_t tss;
u8 task0_sys_stack[1024];
static void setup_tss()
{
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = 0x10;
    tss.esp0 = (u32)&task0_sys_stack + sizeof(task0_sys_stack);

    setup_gdt_entry(SEG_TSS, (u32)&tss, sizeof(tss), GDT_TSS_PL3);
}

extern "C" void flush_tss();

void task0()
{
    u8 i = 0;
    for(;;) {
        if (i < 4) {
            asm ( "int $0x80"::"a"(i));
            ++i;
        }
    }
}


extern "C" int kernel_main(struct multiboot_info *mb)
{
    init_gdt();
    init_idt();
    init_timer();
    init_syscall();

     //check_paging();
    
    set_text_color(LIGHT_GREEN, BLACK);
    const char* msg = "Welcome to SOS....\n";
    kputs(msg);
    u32 memsize = 0;
    if (mb->flags & 0x1) {
        memsize = mb->low_mem + mb->high_mem;
        set_text_color(LIGHT_RED, BLACK);
        kprintf("detected mem(%dKB): low: %dKB, hi: %dKB\n",
                memsize, mb->low_mem, mb->high_mem);
    }

    if (mb->flags & 0x40) {
        memory_map_t* map = (memory_map_t*)p2v(mb->mmap_addr);
        int len = mb->mmap_length / sizeof(memory_map_t);
        kprintf("mmap(%d entries) at 0x%x: \n", len, mb->mmap_addr);
        for (int i = 0; i < len; ++i) {
            kprintf("size: %d, base: 0x%x%x, len: 0x%x%x, type: %d\n",
                    map[i].size, map[i].base_addr_high, 
                    map[i].base_addr_low, map[i].length_high, map[i].length_low,
                    map[i].type);
        }
    }

    if (is_cpuid_capable()) {
        kprintf("4M Page is %ssupported.\n", (is_support_4m_page()?"":"not "));
    }

    PhysicalMemoryManager pmm;
    VirtualMemoryManager* vmm = VirtualMemoryManager::get();

    pmm.init(memsize);
    vmm->init(&pmm);
    Keyboard* kb = Keyboard::get();
    kb->init();

    __asm__ __volatile__ ("sti");

    page_directory_t* pdir = vmm->create_address_space();
    vmm->switch_page_directory(pdir);

     //copy task0 to user-space addr
    void* vaddr = (void*)0x08000000; 
    u32 paddr = v2p(vmm->alloc_page());
    vmm->map_pages(pdir, vaddr, PGSIZE, paddr, PDE_USER|PDE_WRITABLE);
    memcpy(vaddr, (void*)&task0, pmm.frame_size);

    void* task0_usr_stack0 = (void*)0x08100000; 
    u32 paddr_stack0 = v2p(vmm->alloc_page());
    vmm->map_pages(pdir, task0_usr_stack0, PGSIZE, paddr_stack0, PDE_USER|PDE_WRITABLE);
    kprintf("alloc task0: eip 0x%x, stack: 0x%x\n", paddr, paddr_stack0);


    vmm->dump_page_directory(vmm->current_directory());

    setup_tss();
    flush_tss();
    switch_to_usermode((void*)((u32)task0_usr_stack0 + 1024), vaddr);

    panic("Never Back to Here\n");

    return 0x1BADFEED;
}
