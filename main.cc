#include <stddef.h>
#if !defined __cplusplus__
#include <stdbool.h>
#endif
#include <stdint.h>

#if !defined __i386__
#error "target error, need a cross-compiler with ix86-elf as target"
#endif

#include "boot.h"

extern "C" int kernel_main(struct multiboot_info *mb)
{
    clear();
    const char* msg = "Welcome to COS....";
    kputs(msg);
    if (mb->flags & 0x1) {
        u32 memsize = mb->low_mem + mb->high_mem;
        kprintf("detected mem: low: %dKB, hi: %dKB\n", mb->low_mem, mb->high_mem);
    }

    __asm__ __volatile__ ("sti");

    return 0x1BADFEED;
}
