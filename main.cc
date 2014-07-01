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
extern "C" void switch_to_usermode(void* ring3_esp, void* ring3_eip);
extern "C" void flush_tss();
extern void setup_tss(u32);
extern void init_syscall();


/*
 * gets called before Global constructor 
 */
extern "C" void kernel_init()
{
    clear();
    set_text_color(LIGHT_CYAN, BLACK);
}

static bool is_support_4m_page()
{
    u32 edx;
    __asm__ (
        "mov $1, %%eax \n"
        "cpuid \n"
        :"=d"(edx)
        :: "eax"
    );
    return (edx & (1<<4)) > 0;
}

void task0()
{
    u8 step = 0;
    for(;;) {
        int ret = 0;
        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"('A')
                :"cc", "memory");
        ret = ret % 10;
        asm volatile ( "int $0x80"::"a"(SYS_write), "b"('0'+ret));

        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"('0'+ (step%10))
                :"cc", "memory");

        volatile int r = 0;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 0x7fffff; ++j) {
                r += j;
            }
        }

        step++;
    }
}

void task1()
{
    u8 step = 0;
    for(;;) {
        int ret = 0;
        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"('B')
                :"cc", "memory");
        ret = ret % 10;
        asm volatile ( "int $0x80"::"a"(SYS_write), "b"('0'+ret));

        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"('0'+ (step%10))
                :"cc", "memory");

        volatile int r = 0;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 0x7fffff; ++j) {
                r += j;
            }
        }

        step++;
    }
}
extern "C" int kernel_main(struct multiboot_info *mb)
{
    init_gdt();
    init_idt();
    init_timer();
    init_syscall();

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

    proc_t* proc = create_proc((void*)&task0, "task0");
    proc_t* proc1 = create_proc((void*)&task1, "task1");

    vmm->switch_page_directory(proc->pgdir);
    setup_tss(proc->kern_esp);
    flush_tss();
    switch_to_usermode((void*)proc->user_esp, proc->entry);

    panic("Never Back to Here\n");

    return 0x1BADFEED;
}
