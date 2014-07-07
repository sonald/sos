#include "common.h"
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

void task0()
{
    u8 step = 0;
    for(;;) {
        int ret = 0;
        char buf[] = "A";
        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"(0), "c"(buf), "d"(1)
                :"cc", "memory");
        ret = ret % 10;
        buf[0] = '0'+ret;
        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"(0), "c"(buf), "d"(1)
                :"cc", "memory");

        buf[0] = '0'+(step%10);
        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"(0), "c"(buf), "d"(1)
                :"cc", "memory");

        buf[0] = ' ';
        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"(0), "c"(buf), "d"(1)
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
        char buf[] = "B";
        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"(0), "c"(buf), "d"(1)
                :"cc", "memory");
        ret = ret % 10;
        buf[0] = '0'+ret;
        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"(0), "c"(buf), "d"(1)
                :"cc", "memory");

        buf[0] = '0'+(step%10);
        asm volatile ( "int $0x80 \n"
                :"=a"(ret)
                :"a"(SYS_write), "b"(0), "c"(buf), "d"(1)
                :"cc", "memory");

        buf[0] = ' ';
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

static void test_ramfs()
{
    FileSystem* ramfs = devices[RAMFS_MAJOR];
    
    inode_t* ramfs_root = ramfs->root();
    int i = 0;
    dentry_t* de = ramfs->dir_read(ramfs_root, i);
    while (de) {
        inode_t* ip = ramfs->dir_lookup(ramfs_root, de->name);

        kprintf("file %s: [", de->name);
        int buf[64];
        int len = ramfs->read(ip, buf, sizeof buf - 1, 0);
        buf[len] = 0;
        kprintf("%s]\n", buf);

        vmm.kfree(de);
        i++;
        de = ramfs->dir_read(ramfs_root, i);
    }
}

static void load_program(const char* progname)
{
    FileSystem* ramfs = devices[RAMFS_MAJOR];

    inode_t* ramfs_root = ramfs->root();
    inode_t* ip = ramfs->dir_lookup(ramfs_root, progname);

    char* buf = new char[ip->size];
    int len = ramfs->read(ip, buf, ip->size, 0);
    if (len < 0) {
        kprintf("load %s failed\n", progname);
        return;
    }

    elf_header_t* elf = (elf_header_t*)buf;
    if (elf->e_magic != ELF_MAGIC) {
        kprintf("invalid elf file\n");
        return;
    }

    kprintf("elf: 0x%x, entry: 0x%x, ph: %d\n", elf, elf->e_entry, elf->e_phnum);
    elf_prog_header_t* ph = (elf_prog_header_t*)((char*)elf + elf->e_phoff);
    for (int i = 0; i < elf->e_phnum; ++i) {
        kprintf("off: 0x%x, pa: 0x%x, va: 0x%x, fsz: 0x%x, msz: 0x%x\n", 
                ph[i].p_offset, ph[i].p_pa, ph[i].p_va, ph[i].p_filesz, ph[i].p_memsz);
        if (ph[i].p_type != ELF_PROG_LOAD) {
            continue;
        }

        char* prog = (char*)elf + ph[i].p_offset;
        create_proc((void*)elf->e_entry, prog, ph[i].p_memsz, "echo");
        break;
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
    
    //test_ramfs();
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
    if (mb->flags & MULTIBOOT_FLAG_MEM) {
        memsize = mb->low_mem + mb->high_mem;
        set_text_color(LIGHT_RED, BLACK);
        kprintf("detected mem(%dKB): low: %dKB, hi: %dKB\n",
                memsize, mb->low_mem, mb->high_mem);
    }

    if (mb->flags & MULTIBOOT_FLAG_MMAP) {
        apply_mmap(mb->mmap_length, mb->mmap_addr);
    }

    void* last_address = NULL;
    if (mb->flags & MULTIBOOT_FLAG_MODS) {
        module_t* mod = (module_t*)p2v(mb->mods_addr);
        last_address = p2v(mod->mod_end);
    }

    PhysicalMemoryManager pmm;

    pmm.init(memsize, last_address);
    vmm.init(&pmm);

    load_module(mb->mods_count, mb->mods_addr);

    kbd.init();

    proc_t* proc = create_proc((void*)UCODE, (void*)&task0, 1024, "task0");
    create_proc((void*)UCODE, (void*)&task1, 1024, "task1");
    load_program("echo");

    vmm.switch_page_directory(proc->pgdir);
    setup_tss(proc->kern_esp);
    flush_tss();
    switch_to_usermode((void*)proc->user_esp, proc->entry);

    panic("Never Back to Here\n");

    return 0x1BADFEED;
}
