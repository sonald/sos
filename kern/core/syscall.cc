#include "syscall.h"
#include "string.h"
#include "isr.h"
#include "task.h"
#include "spinlock.h"
#include "sys.h"

typedef int (*syscall0_t)();
typedef int (*syscall1_t)(u32);
typedef int (*syscall2_t)(u32, u32);
typedef int (*syscall3_t)(u32, u32, u32);
typedef int (*syscall4_t)(u32, u32, u32, u32);

#define do_syscall0(fn) fn()
#define do_syscall1(fn, arg0) fn(arg0)
#define do_syscall2(fn, arg0, arg1) fn(arg0, arg1)
#define do_syscall3(fn, arg0, arg1, arg2) fn(arg0, arg1, arg2)
#define do_syscall4(fn, arg0, arg1, arg2, arg3) fn(arg0, arg1, arg2, arg3)

static struct syscall_info_s {
    void* call;
    int nr_args;

} syscalls[NR_SYSCALL];

Spinlock syscallock {"syscall"};


extern void check_pending_signal(trapframe_t* regs);
static void syscall_handler(trapframe_t* regs)
{
    auto oflags = syscallock.lock();
    current->regs = regs;
    syscallock.release(oflags);

    if (regs->eax >= NR_SYSCALL) {
        kprintf("invalid syscall number: %d\n", regs->eax);
        regs->eax = -1;
        return;
    }

    struct syscall_info_s info = syscalls[regs->eax];
    if (!info.call) {
        return;
    }

    switch(info.nr_args) {
        case 0:
            {
                syscall0_t fn = (syscall0_t)info.call;
                regs->eax = do_syscall0(fn);
                break;
            }

        case 1:
            {
                syscall1_t fn = (syscall1_t)info.call;
                if (regs->eax == SYS_sigreturn) {
                    regs->eax = do_syscall1(fn, (uint32_t)regs);
                } else 
                    regs->eax = do_syscall1(fn, regs->ebx);
                break;
            }

        case 2:
            {
                syscall2_t fn = (syscall2_t)info.call;
                regs->eax = do_syscall2(fn, regs->ebx, regs->ecx);
                break;
            }

        case 3:
            {
                syscall3_t fn = (syscall3_t)info.call;
                regs->eax = do_syscall3(fn, regs->ebx, regs->ecx,
                        regs->edx);
                break;
            }

        case 4:
            {
                syscall4_t fn = (syscall4_t)info.call;
                regs->eax = do_syscall4(fn, regs->ebx, regs->ecx,
                        regs->edx, regs->esi);
                break;
            }

        default: panic("SYSCALL crash\n");
    }

    if (regs->cs == 0x1b) check_pending_signal(regs);
}

void init_syscall()
{
    memset(syscalls, 0, sizeof(syscalls));

    syscalls[SYS_open] = { (void*)sys_open, 3 };
    syscalls[SYS_close] = { (void*)sys_close, 1 };
    syscalls[SYS_write] = { (void*)sys_write, 3 };
    syscalls[SYS_read] = { (void*)sys_read, 3 };
    syscalls[SYS_readdir] = { (void*)sys_readdir, 3 };
    syscalls[SYS_fork] = { (void*)sys_fork, 0 };
    syscalls[SYS_sleep] = { (void*)sys_sleep, 1 };
    syscalls[SYS_getpid] = { (void*)sys_getpid, 0 };
    syscalls[SYS_getppid] = { (void*)sys_getppid, 0 };
    syscalls[SYS_exec] = { (void*)sys_execve, 3 };
    syscalls[SYS_wait] = { (void*)sys_wait, 1 };
    syscalls[SYS_waitpid] = { (void*)sys_waitpid, 3 };
    syscalls[SYS_exit] = { (void*)sys_exit, 0 };
    syscalls[SYS_mount] = { (void*)sys_mount, 5 };
    syscalls[SYS_umount] = { (void*)sys_unmount, 1 };
    syscalls[SYS_dup] = { (void*)sys_dup, 1 };
    syscalls[SYS_dup2] = { (void*)sys_dup2, 2 };
    syscalls[SYS_pipe] = { (void*)sys_pipe, 1 };
    syscalls[SYS_kdump] = { (void*)sys_kdump, 0 };
    syscalls[SYS_sbrk] = { (void*)sys_sbrk, 1 };
    syscalls[SYS_lseek] = { (void*)sys_lseek, 3 };
    syscalls[SYS_fstat] = { (void*)sys_fstat, 2 };
    syscalls[SYS_stat] = { (void*)sys_stat, 2 };
    syscalls[SYS_lstat] = { (void*)sys_lstat, 2 };
    syscalls[SYS_kill] = { (void*)sys_kill, 2 };
    syscalls[SYS_signal] = { (void*)sys_signal, 2 };
    syscalls[SYS_sigaction] = { (void*)sys_sigaction, 3 };
    syscalls[SYS_sigpending] = { (void*)sys_sigpending, 1 };
    syscalls[SYS_sigprocmask] = { (void*)sys_sigprocmask, 3 };
    syscalls[SYS_sigsuspend] = { (void*)sys_sigsuspend, 1 };
    syscalls[SYS_sigreturn] = { (void*)sys_sigreturn, 1 };

    register_isr_handler(ISR_SYSCALL, syscall_handler);
}

