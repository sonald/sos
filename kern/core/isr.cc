#include "isr.h"
#include "types.h"
#include "task.h"
#include "x86.h"
#include "sched.h"
#include <limits.h>

/* This is a simple string array. It contains the message that
 * *  corresponds to each and every exception. We get the correct
 * *  message by accessing like:
 * *  exception_message[interrupt_number] */
const char* exception_messages[] =
{
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

static interrupt_handler isr_handlers[256];
extern void busy_wait(int millisecs);

#define IO_PIC1         0x20    // Master (IRQs 0-7)
#define IO_PIC2         0xA0    // Slave (IRQs 8-15)
#define IRQ_SLAVE       2       // IRQ at which slave connects to master

// Initial IRQ mask has interrupt 2 enabled (for slave 8259A).
static u16 irqmask = 0xFFFF & ~(1<<IRQ_SLAVE);

static void picsetmask(ushort mask)
{
    irqmask = mask;
    outb(IO_PIC1+1, mask);
    outb(IO_PIC2+1, mask >> 8);
}

void picenable(int irq)
{
    if (irq > IRQ_OFFSET) irq -= IRQ_OFFSET;
    picsetmask(irqmask & ~(1<<irq));
}

void isr_handler(trapframe_t* regs)
{
    if (regs->isrno < 32) kputs(exception_messages[regs->isrno]);
    if (isr_handlers[regs->isrno]) {
        interrupt_handler cb = isr_handlers[regs->isrno];
        cb(regs);
        return;
    }

    kprintf(" regs: ds: 0x%x, edi: 0x%x, esi: 0x%x, ebp: 0x%x, esp: 0x%x\n"
            "ebx: 0x%x, edx: 0x%x, ecx: 0x%x, eax: 0x%x, isr: %d, errno: %d\n"
            "eip: 0x%x, cs: 0x%x, eflags: 0b%b, useresp: 0x%x, ss: 0x%x\n",
            regs->ds, regs->edi, regs->esi, regs->ebp, regs->esp, 
            regs->ebx, regs->edx, regs->ecx, regs->eax, regs->isrno, regs->errcode,
            regs->eip, regs->cs, regs->eflags, regs->useresp, regs->ss);
    for(;;) {
        busy_wait(INT_MAX);
    }
}

void irq_handler(trapframe_t* regs)
{
    if (regs->isrno >= 32 && regs->isrno <= 47) {
        // Send an EOI (end of interrupt) signal to the PICs.
        // If this interrupt involved the slave.
        if (regs->isrno >= 40) {
            // Send reset signal to slave.
            outb(IO_PIC2, 0x20);
        }
        // Send reset signal to master. (As well as slave, if necessary).
        outb(IO_PIC1, 0x20);
    }

    if (isr_handlers[regs->isrno]) {
        interrupt_handler cb = isr_handlers[regs->isrno];
        cb(regs);
    }

    if (regs->isrno == IRQ_TIMER || current->need_resched) 
        scheduler();
}

void register_isr_handler(int isr, interrupt_handler cb)
{
    //TODO: check old handler
    isr_handlers[isr] = cb;
}
