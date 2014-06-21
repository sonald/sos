#include "syscall.h"
#include "isr.h"

static void syscall_handler(registers_t* regs)
{
    kprintf("syscall(%d) ", regs->eax);
}

void init_syscall()
{
    register_isr_handler(0x80, syscall_handler);
}

