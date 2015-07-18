#include <types.h>
#include <spinlock.h>
#include <sos/signal.h>
#include <task.h>
#include <string.h>
#include <syscall.h>

#ifdef DEBUG
#define signal_dbg(fmt,...) kprintf(fmt, ## __VA_ARGS__)
#else
#define signal_dbg(fmt,...)
#endif


#define BLOCKABLE (~(S_MASK(SIGKILL) | S_MASK(SIGSTOP)))

Spinlock siglock {"signal"};

// default actions
enum DFL_ACTION {
    DFL_TERM,
    DFL_COREDUMP,
    DFL_IGNORE,
    DFL_STOP
};

DFL_ACTION dfl_action[NSIG] = {
    DFL_IGNORE, /* 0 */
	DFL_TERM, /* SIGHUP     */
	DFL_TERM, /* SIGINT     */
	DFL_COREDUMP, /* SIGQUIT    */
	DFL_COREDUMP, /* SIGILL     */
	DFL_COREDUMP, /* SIGTRAP    */

	DFL_COREDUMP, /* SIGABRT    */
	DFL_COREDUMP, /* SIGEMT     */
	DFL_COREDUMP, /* SIGFPE     */
	DFL_TERM, /* SIGKILL    */
	DFL_COREDUMP, /* SIGBUS     */

	DFL_COREDUMP, /* SIGSEGV    */
	DFL_COREDUMP, /* SIGSYS     */
	DFL_TERM, /* SIGPIPE    */
	DFL_TERM, /* SIGALRM    */
	DFL_TERM, /* SIGTERM    */

	DFL_TERM, /* SIGUSR1    */
	DFL_TERM, /* SIGUSR2    */
	DFL_IGNORE, /* SIGCHLD    */
	DFL_TERM, /* SIGPWR     */
	DFL_IGNORE, /* SIGWINCH   */

	DFL_IGNORE, /* SIGURG     */
	DFL_IGNORE, /* SIGPOLL    */
	DFL_STOP, /* SIGSTOP    */
	DFL_STOP, /* SIGTSTP    */
	DFL_IGNORE, /* SIGCONT    */

	DFL_STOP, /* SIGTTIN    */
	DFL_STOP, /* SIGTTOU    */
};

int sys_kill(pid_t pid, int sig)
{
    if (sig <= 0 || sig >= NSIG) return -EINVAL;
    if (pid == 1) return -EPERM;

    if (pid > 0) {
        proc_t* tsk = &tasks[0];
        auto oldflags = siglock.lock();
        while (tsk) {
            if (tsk->pid != pid) {
                tsk = tsk->next;
                continue;
            }

            if (tsk->state == TASK_READY || tsk->state == TASK_RUNNING 
                    // sleep but interruptable, maybe use linux style state     
                    // INTERRUPTALE_SLEEP
                    || tsk->state == TASK_SLEEP) {
                tsk->sig.signal |= S_MASK(sig);
                signal_dbg("%s: sigmask 0x%x\n", __func__, tsk->sig.signal);
                break;    
            } 
            tsk = tsk->next;
        }
        siglock.release(oldflags);

    } else if (pid == 0) {
        //TODO: send to pgrp of current
    } else if (pid == -1) {
        //TODO: send to every task except init
    } else {
        //TODO: send to pgrp -pid
    }
    return 0;
}

int sys_signal(int sig, unsigned long handler)
{
    if (sig < 1 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP)
        return -EINVAL;

    //check if handler reside in the text segment
    auto& mm = current->mmap[0];
    if (handler != (uint32_t)SIG_IGN && handler != (uint32_t)SIG_DFL
            && (handler < (uint32_t)mm.start
                || handler >= (uint32_t)mm.start + mm.size)) {
        return (int)SIG_ERR;
    }

    signal_dbg("%s: register sig %d \n", __func__, sig);
    int oldhand = (int)current->sig.action[sig].sa_handler;
    struct sigaction act;
    act.sa_mask = 0;
    act.sa_flags = 0;
    act.sa_handler = (_signal_handler_t)handler;
    current->sig.action[sig] = act;

    return oldhand;
}


int sys_sigaction(int sig, const struct sigaction * act,
	struct sigaction * oldact)
{
    if (sig < 1 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP)
        return -EINVAL;

    if (!act) return -EFAULT;

    if (oldact) {
        *oldact = current->sig.action[sig];
    }
    current->sig.action[sig] = *act;

    return (int)SIG_ERR;
}

int sys_sigpending(sigset_t *set)
{
    if (set) {
        *set = current->sig.blocked & current->sig.signal;
    }
    return 0;
}

int sys_sigprocmask(int how, sigset_t *set, sigset_t *oldset)
{
    if (!set) return -EINVAL;

    sigset_t old = current->sig.blocked;
    sigset_t newset = *set & BLOCKABLE;

    switch(how) {
        case SIG_BLOCK:
            current->sig.blocked |= newset; break;
        case SIG_UNBLOCK:
            current->sig.blocked &= newset; break;
        case SIG_SETMASK:
            current->sig.blocked = newset; break;
        default: return -EINVAL;
    }

    if (oldset) *oldset = old;
    return 0;
}

int sys_sigsuspend(sigset_t *sigmask)
{
    (void)sigmask;
    return 0;
}

//TODO: Will this reentrant?
int sys_sigreturn(uint32_t pad)
{
    signal_dbg("%s: (proc %d(%s))\n", __func__,
            current->pid, current->name);
    trapframe_t* regs = (trapframe_t*)pad;
    // 8 is trampoline code size
    sigcontext_t* ctx = (sigcontext_t*)(regs->useresp + 12);

    regs->eax = ctx->eax;
    regs->ebx = ctx->ebx;
    regs->ecx = ctx->ecx;
    regs->edx = ctx->edx;
    regs->esi = ctx->esi;
    regs->edi = ctx->edi;
    regs->ebp = ctx->ebp;
    regs->useresp = ctx->uesp;
    regs->eip = ctx->eip;
    signal_dbg("REST(0x%x): ds: 0x%x, eax: 0x%x, isr: %d, errno: %d\n"
            "eip: 0x%x, cs: 0x%x, eflags: 0b%b, useresp: 0x%x, ss: 0x%x\n",
            ctx,
            regs->ds, regs->eax, regs->isrno, regs->errcode,
            regs->eip, regs->cs, regs->eflags, regs->useresp, regs->ss);
    return 0;
}

int handle_signal(int sig, trapframe_t* regs)
{
    signal_dbg("%s: (proc %d(%s)) sig %d\n", __func__,
            current->pid, current->name, sig);
    auto& act = current->sig.action[sig];
    _signal_handler_t handler = act.sa_handler;
    if (handler == SIG_IGN) return 0;
    if (handler == SIG_DFL) {
        switch(dfl_action[sig]) {
            case DFL_TERM:
            case DFL_STOP: do_exit(sig);
            case DFL_IGNORE:
            case DFL_COREDUMP:
                break;
        }
        return 0;
    } else {
        uint32_t oesp = regs->useresp;
        uint32_t* ustack = (uint32_t*)((char*)oesp - sizeof(sigcontext_t));
        sigcontext_t ctx;
        ctx.eax = regs->eax;
        ctx.ebx = regs->ebx;
        ctx.ecx = regs->ecx;
        ctx.edx = regs->edx;
        ctx.esi = regs->esi;
        ctx.edi = regs->edi;
        ctx.ebp = regs->ebp;
        ctx.uesp = regs->useresp;
        ctx.eip = regs->eip;
        memcpy(ustack, &ctx, sizeof(sigcontext_t));

        // trampoline code
        // movl SYS_sigreturn, %eax
        // int $0x80
        // ret
        char* code = (char*)ustack - 1;
        *code-- = 0xc3; // ret
        *code-- = 0x80; 
        *code = 0xcd; // int $0x80
        code -= 4;
        *(uint32_t*)code = SYS_sigreturn;
        code--;
        *code = 0xb8;

        signal_dbg("save at 0x%x, code at 0x%x, new eip 0x%x\n",
                ustack, code, handler);
        ustack = (uint32_t*)code - 2;
        ustack[0] = (uint32_t)code;
        ustack[1] = sig;
        regs->eip = (uint32_t)handler;
        regs->useresp = (uint32_t)ustack;

        current->sig.signal &= ~S_MASK(sig);
    }
    return 0;
}

void check_pending_signal(trapframe_t* regs)
{
    auto oldflags = siglock.lock();
    uint32_t mask = current->sig.signal & ~current->sig.blocked;
    if (mask && current->state != TASK_SLEEP) {
        signal_dbg("%s: during isr #%d\n", __func__, regs->isrno);
        for (int i = 1; i < NSIG; i++) {
            if (mask & S_MASK(i)) {
                handle_signal(i, regs);
                break;
            }
        }
    }
    siglock.release(oldflags);
}
