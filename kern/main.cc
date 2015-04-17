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
#include "fat32.h"
#include <sprintf.h>
#include <dirent.h>
#include <sys.h>
#include <string.h>

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
    //current_display->clear();
}

void idle_thread()
{
    asm volatile (
        "1: hlt \n"
        "jmp 1b");
}

static void debug_print(Keyboard* kb)
{
    key_packet_t pkt = kb->kbbuf().read();
    KeyCode key = pkt.keycode;
    if (key == KEY_UNKNOWN || (pkt.status & KB_RELEASE))
        return;

    if (kb->shift_down()) {
        if (key >= KEY_A && key <= KEY_Z) {
            kputchar(key - 0x20);
        } else if (key >= KEY_0 && key <= KEY_9){
        }
    } else {
        if (key == KEY_RETURN)
            kputchar('\n');
        else if (key == KEY_TAB)
            kputchar('\t');
        else if (key == KEY_BACKSPACE)
            kputchar('\b');
        else
            kputchar(key);
    }
}

void tty_thread()
{
    
    while (1) {
        if (!kbd.msbuf().empty()) {
            mouse_packet_t pkt = kbd.msbuf().read();

            auto old = current_display->get_text_color();
            auto cur = current_display->get_cursor();
            cli();
            current_display->set_text_color(LIGHT_BLUE);
            current_display->set_cursor({60, 2});

            kprintf(" @%d, %d, 0x%x@ ", pkt.relx, pkt.rely, pkt.flags);

            current_display->set_cursor(cur);
            current_display->set_text_color(old);
            sti();
        }

        if (!kbd.kbbuf().empty()) {
            debug_print(&kbd);
        }

        asm ("hlt");
    }
}

void kthread1()
{
    dev_t ROOT_DEV = DEVNO(IDE_MAJOR, 0);
    Buffer* mbr = bio.read(ROOT_DEV, 0);

    bool released = false;
    int count = 0;
    while (1) {
        auto cur = current_display->get_cursor();
        current_display->set_cursor({20, 20});
        kprintf(" <KT1 %d> ", count++);
        current_display->set_cursor(cur);

        sys_sleep(2000);
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
        sys_sleep(2000);
        auto cur = current_display->get_cursor();
        current_display->set_cursor({0, 20});
        kprintf(" |KT2 %d| ", count++);
        current_display->set_cursor(cur);

        dev_t ROOT_DEV = DEVNO(IDE_MAJOR, 0);
        Buffer* mbr = bio.read(ROOT_DEV, 0);
        bio.release(mbr);
    }
}

void init_task()
{
    char init_name[] = "/init";
    asm volatile ( "int $0x80 \n"
            :
            :"a"(SYS_exec), "b"(init_name), "c"(0), "d"(0)
            :"cc", "memory");
    panic("should never come here");
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

static void test_fs_read(const char* filepath) 
{       
        int fd = sys_open(filepath, O_RDONLY, 0);
        if (fd >= 0) {
            char buf[64];
            int len = 0;
            while ((len = sys_read(fd, buf, sizeof buf - 1)) > 0) {
                buf[len] = 0;
                kputs(buf);
            }                
            sys_close(fd); 
        }            
}

static void test_fs_readdir(const char* dir)
{
    int fd = sys_open(dir, O_RDONLY, 0);
    kassert(fd >= 0);
    struct dirent dire;
    while (sys_readdir(fd, &dire, 1) > 0) {
        kprintf("%s, ", dire.d_name);        
    }
    sys_close(fd);
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
    
    ramfs_mod_info_t info;
    info.addr = mod_start;
    info.size = mod_end - mod_start;
    info.cmdline = (char*)p2v(mod->string);
    vfs.mount("ramfs", "/ram", "ramfs", 0, (void*)&info);
}

extern "C" int kernel_main(struct multiboot_info *mb)
{
    init_gdt();
    init_idt();
    init_timer();
    init_syscall();

    u32 memsize = 0;
    if (mb->flags & MULTIBOOT_INFO_MEMORY) {
        memsize = mb->low_mem + mb->high_mem;
        current_display->set_text_color(LIGHT_RED);
    }

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
            current_display = &graph_console;
            current_display->clear();
            //video_mode_test();
            kprintf("VideoMode:\n "
                    "winsize: %d, pitch: %d, xres: %d, yres: %d,"
                    "planes: %d, banks: %d, bank_size: %d\n",
                    modinfo->winsize, modinfo->pitch, modinfo->Xres,
                    modinfo->Yres, modinfo->planes, modinfo->banks,
                    modinfo->bank_size);
        }
    }
    current_display->set_text_color(CYAN);
    const char* msg = "booting SOS....\n";
    kputs(msg);
    current_display->set_text_color(YELLOW);
    kprintf("detected mem(%dKB): low: %dKB, hi: %dKB\n",
            memsize, mb->low_mem, mb->high_mem);
    if (mb->flags & MULTIBOOT_INFO_MEM_MAP) {
        apply_mmap(mb->mmap_length, mb->mmap_addr);
    }
    current_display->set_text_color(LIGHT_MAGENTA);

    tasks_init();
    kbd.init(); // both kb & mouse
    ata_init();
    bio.init();

    vfs.register_fs("fat32", create_fat32fs);
    vfs.register_fs("ramfs", create_ramfs);
    vfs.init_root(DEVNO(IDE_MAJOR, 1));

    picenable(IRQ_KBD);
    picenable(IRQ_MOUSE);
    picenable(IRQ_TIMER);

    proc_t* proc = prepare_userinit((void*)&init_task);
    load_module(mb->mods_count, mb->mods_addr);

    // test_fs_readdir("/boot/grub/i386-pc");
    // for(;;) asm volatile ("hlt");
 
    // char filename[NAMELEN+1] = "/boot/grub/grub.cfg";
    // char filename[NAMELEN+1] = "/longnamedmsdosfile.txt";     
    // test_fs_read(filename);
    // for(;;) asm volatile ("hlt");

    create_kthread("kthread1", kthread1);
    create_kthread("kthread2", kthread2);
    create_kthread("idle", idle_thread);
    create_kthread("tty", tty_thread);

    vmm.switch_page_directory(proc->pgdir);
    setup_tss(proc->kern_esp);
    flush_tss();
    switch_to_usermode((void*)proc->user_esp, proc->entry);

    panic("Never Back to Here\n");

    return 0x1BADFEED;
}
