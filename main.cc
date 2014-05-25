#include "common.h"
#include "boot.h"

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

extern "C" int kernel_main(struct multiboot_info *mb)
{
    // need guard to use this
    static Manager man2;

    set_text_color(LIGHT_GREEN, BLACK);
    const char* msg = "Welcome to SOS....\n";

    // check if paging is working correctly
    u32 kernel_base = 0xC0000000;
    for (u32 i = 0; i < 1024; ++i) {
        kprintf("%x", *(u32*)(kernel_base + i*4096 + 4));
    }

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

    __asm__ __volatile__ ("sti");

    return 0x1BADFEED;
}
