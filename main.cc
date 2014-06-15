#include "common.h"
#include "boot.h"
#include "isr.h"
#include "timer.h"
#include "mm.h"
#include "vm.h"

struct Manager 
{
    char name[20];
    Manager() {
        static int count = 1;
        memcpy(name, "hello", 5);
        name[5] = '\0';
        kprintf("Manager %d\n", count++);
    }
    ~Manager() {
        kputs("destructor!\n");
    }
};

Manager man1;

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
    kprintf("base: 0x%x\n", kernel_virtual_base);

    // check if paging is working correctly
    for (u32 i = 0; i < 1024; ++i) {
        kprintf("%x", *(u32*)(kernel_virtual_base + i*4096 + 4));
    }
}

static void test_pmm(PhysicalMemoryManager& pmm)
{
    void* p = pmm.alloc_frame();
    if (p == NULL) kputs("alloc failed\n");
    pmm.free_frame(p);
    void* p2 = pmm.alloc_frame();
    if (p != p2) kputs("p != p2\n");

    u32 size = pmm.frame_size * 10;
    p = pmm.alloc_region(size);
    if (p == NULL) kputs("alloc region failed\n");
    pmm.free_region(p, size);
    p2 = pmm.alloc_frame();
    if (p != p2) kputs("p != p2\n");
}

extern u32* _end;

extern "C" int kernel_main(struct multiboot_info *mb)
{
    // need guard to use this
    static Manager man2;

    init_gdt();
    init_idt();
    init_timer();

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

    if (is_cpuid_capable()) {
        kprintf("4M Page is %ssupported.\n", (is_support_4m_page()?"":"not "));
    }

    u32 mmap_addr = (u32)&_end;
    PhysicalMemoryManager pmm(memsize, mmap_addr);
    //test_pmm(pmm);
    VirtualMemoryManager vmm(pmm);
    vmm.init();
    vmm.dump_page_directory(vmm.current_directory());

    __asm__ __volatile__ ("sti");

    int i = 0;
    for (;;) {
        busy_wait(1000);
        kprintf("loop %d\r", i++);
    }

    //test_irqs();
    
    for(;;);
    return 0x1BADFEED;
}
