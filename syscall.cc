#include "syscall.h"
#include "isr.h"
#include "task.h"

int sys_write(registers_t* regs)
{
    kputchar(regs->ebx);
    return 1;
}

typedef int (*syscall_cb)(registers_t*);
static syscall_cb syscalls[NR_SYSCALL]; 

static void syscall_handler(registers_t* regs)
{
    int n = ARRAYLEN(syscalls);
    if (regs->eax >= (u32)n) {
        kprintf("invalid syscall number: %d\n", regs->eax);
        current_proc->regs.eax = -1;
        return;
    }
    
    syscall_cb f = syscalls[regs->eax];
    if (f) {
        current_proc->regs.eax = f(regs);
    }
}

void init_syscall()
{
    memset(syscalls, 0, sizeof(syscalls));
    syscalls[SYS_write] = sys_write;
    register_isr_handler(ISR_SYSCALL, syscall_handler);
}

