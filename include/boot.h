#ifndef _BOOT_H
#define _BOOT_H 

#include "common.h"


/* Flags to be set in the 'flags' member of the multiboot info structure.  */

/* is there basic lower/upper memory information? */
#define MULTIBOOT_INFO_MEMORY           0x00000001
/* is there a boot device set? */
#define MULTIBOOT_INFO_BOOTDEV          0x00000002
/* is the command-line defined? */
#define MULTIBOOT_INFO_CMDLINE          0x00000004
/* are there modules to do something with? */
#define MULTIBOOT_INFO_MODS         0x00000008

/* These next two are mutually exclusive */

/* is there a symbol table loaded? */
#define MULTIBOOT_INFO_AOUT_SYMS        0x00000010
/* is there an ELF section header table? */
#define MULTIBOOT_INFO_ELF_SHDR         0X00000020

/* is there a full memory map? */
#define MULTIBOOT_INFO_MEM_MAP          0x00000040

/* Is there drive info?  */
#define MULTIBOOT_INFO_DRIVE_INFO       0x00000080

/* Is there a config table?  */
#define MULTIBOOT_INFO_CONFIG_TABLE     0x00000100

/* Is there a boot loader name?  */
#define MULTIBOOT_INFO_BOOT_LOADER_NAME     0x00000200

/* Is there a APM table?  */
#define MULTIBOOT_INFO_APM_TABLE        0x00000400

/* Is there video information?  */
#define MULTIBOOT_INFO_VBE_INFO             0x00000800
#define MULTIBOOT_INFO_FRAMEBUFFER_INFO         0x00001000



/* The Multiboot header. */
typedef struct multiboot_header
{
    unsigned long magic;
    unsigned long flags;
    unsigned long checksum;
    unsigned long header_addr;
    unsigned long load_addr;
    unsigned long load_end_addr;
    unsigned long bss_end_addr;
    unsigned long entry_addr;
} multiboot_header_t;

typedef struct multiboot_info {
    u32 flags;
    u32 low_mem;
    u32 high_mem;
    u32 boot_device;
    u32 cmdline;
    u32 mods_count;
    u32 mods_addr;
    struct {
        u32 num;
        u32 size;
        u32 addr;
        u32 shndx;
    } elf_sec;
    unsigned long mmap_length;
    unsigned long mmap_addr;
    unsigned long drives_length;
    unsigned long drives_addr;
    unsigned long config_table;
    unsigned long boot_loader_name;
    unsigned long apm_table;
    unsigned long vbe_control_info;
    unsigned long vbe_mode_info;
    unsigned long vbe_mode;
    unsigned long vbe_interface_seg;
    unsigned long vbe_interface_off;
    unsigned long vbe_interface_len;

    uint64_t fb_addr;
    uint32_t fb_pitch;
    uint32_t fb_width;
    uint32_t fb_height;
    uint8_t fb_bpp;
} multiboot_info_t;

/* The module structure. */
typedef struct module
{
    unsigned long mod_start;
    unsigned long mod_end;
    unsigned long string;
    unsigned long reserved;
} module_t;

/* The memory map. Be careful that the offset 0 is base_addr_low
 *         but no size. */
typedef struct memory_map_s {
    unsigned long size;
    unsigned long base_addr_low;
    unsigned long base_addr_high;
    unsigned long length_low;
    unsigned long length_high;
    unsigned long type;
} memory_map_t;

#endif
