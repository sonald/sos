#include "common.h"
#include "boot.h"

extern "C" int kernel_main(struct multiboot_info *mb)
{
    clear();
    set_text_color(LIGHT_GREEN, BLACK);
    const char* msg = "Welcome to SOS....\n";
    kputs(msg);
    if (mb->flags & 0x1) {
        u32 memsize = mb->low_mem + mb->high_mem;
        set_text_color(LIGHT_RED, BLACK);
        kprintf("detected mem: low: %dKB, hi: %dKB\n", mb->low_mem, mb->high_mem);
    }

    __asm__ __volatile__ ("sti");

    return 0x1BADFEED;
}
