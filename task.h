#ifndef _TASK_H
#define _TASK_H 

#include "common.h"

struct tss_entry_
{
    u32 prev_tss;   // The previous TSS - if we used hardware task switching this would form a linked list.
    u32 esp0;       // The stack pointer to load when we change to kernel mode.
    u32 ss0;        // The stack segment to load when we change to kernel mode.
    u32 esp1;       // Unused...
    u32 ss1;
    u32 esp2;
    u32 ss2;
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
    u32 es;         // The value to load into ES when we change to kernel mode.
    u32 cs;         // The value to load into CS when we change to kernel mode.
    u32 ss;         // The value to load into SS when we change to kernel mode.
    u32 ds;         // The value to load into DS when we change to kernel mode.
    u32 fs;         // The value to load into FS when we change to kernel mode.
    u32 gs;         // The value to load into GS when we change to kernel mode.
    u32 ldt;        // Unused...
    u16 trap;
    u16 iomap_base;
};

typedef struct tss_entry_ tss_entry_t;

#endif
