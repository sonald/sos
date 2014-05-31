#include "common.h"
#include "boot.h"
#include "isr.h"
#include "timer.h"

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
    // check if paging is working correctly
    u32 kernel_base = 0xC0000000;
    for (u32 i = 0; i < 1024; ++i) {
        kprintf("%x", *(u32*)(kernel_base + i*4096 + 4));
    }
}

extern "C" int kernel_main(struct multiboot_info *mb)
{
    // need guard to use this
    static Manager man2;
    init_gdt();
    init_idt();
    init_timer();

    __asm__ __volatile__ ("sti");

    // check_paging();
    
    set_text_color(LIGHT_GREEN, BLACK);
    const char* msg = "Welcome to SOS....\n";
    kputs(msg);
    if (mb->flags & 0x1) {
        //u32 memsize = mb->low_mem + mb->high_mem;
        set_text_color(LIGHT_RED, BLACK);
        kprintf("detected mem: low: %dKB, hi: %dKB\n", mb->low_mem, mb->high_mem);
    }

    if (is_cpuid_capable()) {
        kprintf("4M Page is %ssupported.\n", (is_support_4m_page()?"":"not "));
    }

    kputs(man2.name);
    kprintf("0b%b, 0b%b, %d, %u, 0x%x\n", (int)-2, 24+2, -1892, 0xf0000001, 0xfff00000);
    int i = 0;
    for (;;) {
        busy_wait(1000);
        kprintf("loop %d\r", i++);
    }

    //test_irqs();
    
    for(;;);
    return 0x1BADFEED;
}
