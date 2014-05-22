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
        kputs("done!");
    }
};

Manager man1;
Manager man2;

/*
 * gets called before Global constructor 
 */
extern "C" void kernel_init()
{
    clear();
    set_text_color(LIGHT_CYAN, BLACK);
}

extern "C" int kernel_main(struct multiboot_info *mb)
{
    set_text_color(LIGHT_GREEN, BLACK);
    const char* msg = "Welcome to SOS....\n";
    kputs(msg);
    if (mb->flags & 0x1) {
        //u32 memsize = mb->low_mem + mb->high_mem;
        set_text_color(LIGHT_RED, BLACK);
        kprintf("detected mem: low: %dKB, hi: %dKB\n", mb->low_mem, mb->high_mem);
    }

    kputs(man1.name);
    __asm__ __volatile__ ("sti");

    return 0x1BADFEED;
}
