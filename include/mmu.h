#ifndef _MMU_H
#define _MMU_H

#include "types.h"

// This file contains definitions for the
// x86 memory management unit (MMU).

// Control Register flags
#define CR0_PE          0x00000001      // Protection Enable
#define CR0_MP          0x00000002      // Monitor coProcessor
#define CR0_EM          0x00000004      // Emulation
#define CR0_TS          0x00000008      // Task Switched
#define CR0_ET          0x00000010      // Extension Type
#define CR0_NE          0x00000020      // Numeric Errror
#define CR0_WP          0x00010000      // Write Protect
#define CR0_AM          0x00040000      // Alignment Mask
#define CR0_NW          0x20000000      // Not Writethrough
#define CR0_CD          0x40000000      // Cache Disable
#define CR0_PG          0x80000000      // Paging

#define CR4_PSE         0x00000010      // Page size extension

// GDT Segment index
#define SEG_NULL  0  // NULL
#define SEG_KCODE 1  // kernel code
#define SEG_KDATA 2  // kernel data+stack
#define SEG_UCODE 3  // user code
#define SEG_UDATA 4  // user data+stack
#define SEG_TSS   5  // this process's task state


// A virtual address 'la' has a three-part structure as follows:
//
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/


typedef struct page_s {
    u32 present    : 1;   // Page present in memory
    u32 rw         : 1;   // Read-only if clear, readwrite if set
    u32 user       : 1;   // Supervisor level only if clear
    u32 accessed   : 1;   // Has the page been accessed since last refresh?
    u32 dirty      : 1;   // Has the page been written to since last refresh?
    u32 unused     : 7;   // Amalgamation of unused and reserved bits
    u32 frame      : 20;  // Frame address (shifted right 12 bits)
} page_t;

typedef struct page_table_s {
    page_t pages[1024];
} page_table_t;

// for PDE and PTE
enum PAGE_PDE_FLAGS {
    PDE_PRESENT     = 0x01,
    PDE_WRITABLE    = 0x02,
    PDE_USER        = 0x04,
    PDE_PWT         = 0x08,
    PDE_PCD         = 0x10,
    PDE_ACCESSED    = 0x20,
    PDE_DIRTY       = 0x40,
    PDE_4MB         = 0x80,
    PDE_FRAME       = 0xFFFFF000
};

#define pde_set_flag(pde, flag) ((pde) |= (flag))
#define pde_get_flags(pde) (((u32)pde) & ~PDE_FRAME)
#define pde_set_frame(pde, frame) (pde = ((pde) & ~PDE_FRAME) | (u32)frame)

typedef u32 pde_t;

typedef struct page_directory_s {
    pde_t tables[1024];
} page_directory_t;


#define PGSHIFT         12      // log2(PGSIZE)
#define PTXSHIFT        12      // offset of PTX in a linear address
#define PDXSHIFT        22      // offset of PDX in a linear address
#define PGSIZE          4096    // bytes mapped by a page

#define PAGE_DIR_IDX(vaddr) ((((u32)vaddr) >> PDXSHIFT) & 0x3ff)
#define PAGE_TABLE_IDX(vaddr) ((((u32)vaddr) >> PTXSHIFT) & 0x3ff)
#define PAGE_ENTRY_OFFSET(vaddr) (((u32)vaddr) & 0x00000fff)

#define PGROUNDUP(sz)  ((((u32)sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((u32)a) & ~(PGSIZE-1))

#define PDE_GET_TABLE_PHYSICAL(pde) (((pde_t)pde) & 0xfffff000)

// construct virtual address from indexes and offset
#define PGADDR(d, t, o) ((uint)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

#define A2I(addr) ((u32)(unsigned long)(addr))
#define I2A(paddr) ((void*)(unsigned long)(paddr))

struct tss_entry_
{
    u32 prev_tss;   // The previous TSS - if we used hardware task switching this would form a linked list.
    u32 esp0;       // The stack pointer to load when we change to kernel mode.
    u16 ss0;        // The stack segment to load when we change to kernel mode.
    u16 pad1;
    u32 esp1;       // Unused...
    u16 ss1;
    u16 pad3;
    u32 esp2;
    u16 ss2;
    u16 pad5;

    u32 cr3;
    u32 eip;
    u32 eflags;
    u32 eax;
    u32 ecx;
    u32 edx;
    u32 ebx;
    u32 esp;
    u32 ebp;
    u32 esi;
    u32 edi;

    u16 es;         // The value to load into ES when we change to kernel mode.
    u16 pad6;
    u16 cs;         // The value to load into CS when we change to kernel mode.
    u16 pad7;
    u16 ss;         // The value to load into SS when we change to kernel mode.
    u16 pad8;
    u16 ds;         // The value to load into DS when we change to kernel mode.
    u16 pad9;
    u16 fs;         // The value to load into FS when we change to kernel mode.
    u16 pad10;
    u16 gs;         // The value to load into GS when we change to kernel mode.
    u16 pad11;
    u16 ldt;        // Unused...
    u16 pad12;
    u16 trap;
    u16 iomap_base;
};

typedef struct tss_entry_ tss_entry_t;

#endif
