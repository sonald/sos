#ifndef _ISR_H
#define _ISR_H 

#include "common.h"
#include "gdt.h"

#define PAGE_FAULT 14

#define IRQ_OFFSET 32 // offset off ISRs

#define IRQ0 32 // PIT
#define IRQ_TIMER 32 // PIT
#define IRQ1 33 // KBD
#define IRQ_KBD 33
#define IRQ2 34 // slave
#define IRQ3 35 // serial2
#define IRQ4 36 // serial1
#define IRQ5 37 // LPT2
#define IRQ6 38 // floppy
#define IRQ7 39 // LPT1

#define IRQ8  40 // RTC
#define IRQ9  41 // IRQ2
#define IRQ10 42 // reserve
#define IRQ11 43 // reserve
#define IRQ12 44 // PS/2 mouse
#define IRQ13 45 // FPU
#define IRQ_ATA1 46 // ATA HD1
#define IRQ_ATA2 47 // ATA HD2

#define ISR_SYSCALL 0x80 

typedef struct trapframe_s {
    u32 fs, gs, es, ds;
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
    u32 isrno, errcode;
    // Pushed by the processor automatically.
    u32 eip, cs, eflags;
    u32 useresp, ss;  // exists only when PL switched
} __attribute__((packed)) trapframe_t;

typedef struct kcontext_s {
    u32 ebx, esi, edi, ebp, eip;
} __attribute__((packed)) kcontext_t;

BEGIN_CDECL

void isr_handler(trapframe_t* regs);
void irq_handler(trapframe_t* regs);

END_CDECL

void picenable(int irq);

typedef void (*interrupt_handler)(trapframe_t* regs);
void register_isr_handler(int isr, interrupt_handler cb);

#endif
