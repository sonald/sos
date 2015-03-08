#include "syscall.h"
#include "string.h"
#include "isr.h"
#include "task.h"

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

extern int sys_open (const char *path, int flags, int mode);
extern int sys_write(int fildes, const void *buf, size_t nbyte);
extern int sys_close(int fd);
extern int sys_read(int fildes, void *buf, size_t nbyte);
extern int sys_fork();
extern int sys_sleep();
extern int sys_execve(const char *path, char *const argv[], char *const envp[]);
extern int sys_getpid();

static struct syscall_info_s {
    void* call;
    int nr_args;

} syscalls[NR_SYSCALL]; 

static void syscall_handler(trapframe_t* regs)
{
    if (regs->eax >= NR_SYSCALL) {
        kprintf("invalid syscall number: %d\n", regs->eax);
        regs->eax = -1;
        return;
    }
    
    struct syscall_info_s info = syscalls[regs->eax];
    if (!info.call) return;

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
}

void init_syscall()
{
    memset(syscalls, 0, sizeof(syscalls));

    syscalls[SYS_open] = { (void*)sys_write, 3 };
    syscalls[SYS_close] = { (void*)sys_close, 1 };
    syscalls[SYS_write] = { (void*)sys_write, 3 };
    syscalls[SYS_read] = { (void*)sys_read, 3 };
    syscalls[SYS_fork] = { (void*)sys_fork, 0 };
    syscalls[SYS_sleep] = { (void*)sys_sleep, 0 };
    syscalls[SYS_getpid] = { (void*)sys_getpid, 0 };
    syscalls[SYS_exec] = { (void*)sys_execve, 3 };

    register_isr_handler(ISR_SYSCALL, syscall_handler);
}

