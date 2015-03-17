#include "common.h"
#include "x86.h"
#include "boot.h"
#include "gdt.h"
#include "isr.h"
#include "timer.h"
#include "vm.h"
#include "kb.h"
#include "task.h"
#include "syscall.h"
#include "vfs.h"
#include "ramfs.h"
#include "elf.h"
#include "ata.h"
#include "blkio.h"
#include "sched.h"
#include "graphics.h"
#include "display.h"

extern "C" void switch_to_usermode(void* ring3_esp, void* ring3_eip);
extern "C" void flush_tss();
extern void setup_tss(u32);
extern void init_syscall();
extern int sys_write(int fildes, const void *buf, size_t nbyte);


/*
 * gets called before Global constructor 
 */
extern "C" void kernel_init()
{
    current_display->clear();
    //set_text_color(COLOR(LIGHT_CYAN, BLACK));
}

void idle_thread()
{
    asm volatile (
        "1: hlt \n"
        "jmp 1b");
}

void kthread1()
{
    dev_t ROOT_DEV = DEVNO(IDE_MAJOR, 0);
    Buffer* mbr = bio.read(ROOT_DEV, 0);
    if (mbr) {
        kprintf("PART: ");
        uint32_t* p = (uint32_t*)&mbr->data[0x1c0];
        for (size_t i = 0; i < 4; i++, p++) {
            kprintf("%x ", *p);
        }
        kputchar('\n');
    }

    bool released = false;
    int count = 0;
    while (1) {
        kprintf(" <KT1 %d> ", count++);
        busy_wait(2000);
        if (!released) {
            bio.release(mbr);
            released = true;
        }
    }
}

void kthread2()
{
    int count = 0;
    while (1) {
        busy_wait(2000);
        kprintf(" |KT2 %d| ", count++);
        dev_t ROOT_DEV = DEVNO(IDE_MAJOR, 0);
        Buffer* mbr = bio.read(ROOT_DEV, 0);
        kputs(" |KT2: read| ");
        bio.release(mbr);
    }
}

void init_task()
{
    char init_name[] = "echo";
    u8 step = 0;
    char buf[2] = "";
    int logo = 'A';
    pid_t pid;
    asm volatile ( "int $0x80 \n" :"=a"(pid) :"a"(SYS_fork) :"cc", "memory");

    {
        volatile int r = 0;
        for (int i = 0; i < 1; i++) {
            for (int j = 0; j < 0x7fffff; ++j) {
                r += j;
            }
        }
    }

    if (pid == 0) { 
        asm volatile ( "int $0x80 \n"
                :"=a"(pid) 
                :"a"(SYS_exec), "b"(init_name), "c"(0), "d"(0)
                :"cc", "memory");
        for(;;);
        return;
    }

    asm volatile ( "int $0x80 \n" :"=a"(pid) :"0"(SYS_getpid) :"cc", "memory");

    for(;;) {
        int ret = 0;
        buf[0] = logo;
        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"(0), "c"(buf), "d"(1)
                :"cc", "memory");
        buf[0] = '0'+pid;
        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"(0), "c"(buf), "d"(1)
                :"cc", "memory");

        buf[0] = '0'+(step%10);
        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"(0), "c"(buf), "d"(1)
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

static void apply_mmap(u32 mmap_length, u32 mmap_addr) 
{
    memory_map_t* map = (memory_map_t*)p2v(mmap_addr);
    int len = mmap_length / sizeof(memory_map_t);
    kprintf("mmap(%d entries) at 0x%x: \n", len, mmap_addr);
    for (int i = 0; i < len; ++i) {
        kprintf("size: %d, base: 0x%x%x, len: 0x%x%x, type: %d\n",
                map[i].size, map[i].base_addr_high, 
                map[i].base_addr_low, map[i].length_high, map[i].length_low,
                map[i].type);
    }
}

static SOS_UNUSED void test_ramfs(bool content) 
{
    FileSystem* ramfs = devices[RAMFS_MAJOR];
    
    inode_t* ramfs_root = ramfs->root();
    int i = 0;
    dentry_t* de = ramfs->dir_read(ramfs_root, i);
    while (de) {
        inode_t* ip = ramfs->dir_lookup(ramfs_root, de->name);

        kprintf("file %s: ", de->name);
        if (content) {
            kputchar('[');
            int buf[64];
            int len = ramfs->read(ip, buf, sizeof buf - 1, 0);
            buf[len] = 0;
            kprintf("%s]", buf);
        }
        kputchar('\n');

        vmm.kfree(de);
        i++;
        de = ramfs->dir_read(ramfs_root, i);
    }
}

// only care about 1 module
static void load_module(u32 mods_count, u32 mods_base)
{
    if (mods_count == 0) return;
    module_t* mod = (module_t*)p2v(mods_base);
    char* mod_start = (char*)p2v(mod->mod_start);
    char* mod_end = (char*)p2v(mod->mod_end);
    kprintf("load initrd at [0x%x, 0x%x], cmdline: %s\n", mod_start, mod_end,
            p2v(mod->string));
    
    Ramfs* ramfs = new Ramfs;
    ramfs->init(mod_start, mod_end-mod_start, (const char*)p2v(mod->string));
    
    test_ramfs(false);
}

extern "C" int kernel_main(struct multiboot_info *mb)
{
    init_gdt();
    init_idt();
    init_timer();
    init_syscall();

    current_display->set_text_color(COLOR(LIGHT_GREEN, BLACK));
    const char* msg = "Welcome to SOS....\n";
    kputs(msg);
    u32 memsize = 0;
    if (mb->flags & MULTIBOOT_INFO_MEMORY) {
        memsize = mb->low_mem + mb->high_mem;
        current_display->set_text_color(COLOR(LIGHT_RED, BLACK));
        kprintf("detected mem(%dKB): low: %dKB, hi: %dKB\n",
                memsize, mb->low_mem, mb->high_mem);
    }

    if (mb->flags & MULTIBOOT_INFO_MEM_MAP) {
        apply_mmap(mb->mmap_length, mb->mmap_addr);
    }
    current_display->set_text_color(COLOR(LIGHT_GREEN, BLACK));

    void* last_address = NULL;
    if (mb->flags & MULTIBOOT_INFO_MODS) {
        module_t* mod = (module_t*)p2v(mb->mods_addr);
        last_address = p2v(mod->mod_end);
    }

    pmm.init(memsize, last_address);
    vmm.init(&pmm);
    if (mb->flags & MULTIBOOT_INFO_VBE_INFO) {
        ModeInfoBlock_t* modinfo = (ModeInfoBlock_t*)
            (mb->vbe_mode_info+KERNEL_VIRTUAL_BASE);

        if (modinfo->attributes & 0x80) {
            videoMode.init(modinfo);
            char* video = (char*)0xE0000000;
            for (int row = 4; row < 100; row++) {
                for (int j = 0; j < 300; j+=3) {
                    *(video + row*modinfo->pitch + j) = 0xff;
                    *(video + row*modinfo->pitch + j+1) = 0x80;
                    *(video + row*modinfo->pitch + j+2) = 0x08;
                }
            }
            current_display = &graph_console;
            videoMode.drawLine(300, 100, 500, 200, {0xff, 0x00, 0x00});
        }
        for(;;) asm ("cli; hlt");
    }

    tasks_init();
    kbd.init();
    ata_init();
    bio.init();

    picenable(IRQ_KBD);
    picenable(IRQ_TIMER);

    //for(;;) asm volatile ("hlt");

    load_module(mb->mods_count, mb->mods_addr);

    proc_t* proc = prepare_userinit((void*)&init_task);

    create_kthread("kthread1", kthread1);
    create_kthread("kthread2", kthread2);
    create_kthread("idle", idle_thread);

    vmm.switch_page_directory(proc->pgdir);
    setup_tss(proc->kern_esp);
    flush_tss();
    switch_to_usermode((void*)proc->user_esp, proc->entry);

    panic("Never Back to Here\n");

    return 0x1BADFEED;
}
