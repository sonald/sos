#ifndef _SOS_SIGNAL_H
#define _SOS_SIGNAL_H
#include <types.h>

#define SIGHUP      1  /* Hangup */
#define SIGINT      2  /* Interupt */
#define SIGQUIT     3  /* Quit */
#define SIGILL      4  /* Illegal instruction */
#define SIGTRAP     5  /* A breakpoint or trace instruction has been reached */
#define SIGABRT     6  /* Another process has requested that you abort */
#define SIGEMT      7  /* Emulation trap XXX */
#define SIGFPE      8  /* Floating-point arithmetic exception */
#define SIGKILL     9  /* You have been stabbed repeated with a large knife */
#define SIGBUS      10 /* Bus error (device error) */
#define SIGSEGV     11 /* Segmentation fault */
#define SIGSYS      12 /* Bad system call */
#define SIGPIPE     13 /* Attempted to read or write from a broken pipe */
#define SIGALRM     14 /* This is your wakeup call. */
#define SIGTERM     15 /* You have been Schwarzenegger'd */
#define SIGUSR1     16 /* User Defined Signal #1 */
#define SIGUSR2     17 /* User Defined Signal #2 */
#define SIGCHLD     18 /* Child status report */
#define SIGPWR      19 /* We need more power! */
#define SIGWINCH    20 /* Your containing terminal has changed size */
#define SIGURG      21 /* An URGENT! event (On a socket) */
#define SIGPOLL     22 /* XXX OBSOLETE; socket i/o possible */
#define SIGSTOP     23 /* Stopped (signal) */
#define SIGTSTP     24 /* ^Z (suspend) */
#define SIGCONT     25 /* Unsuspended (please, continue) */
#define SIGTTIN     26 /* TTY input has stopped */
#define SIGTTOU     27 /* TTY output has stopped */

#define NUMSIGNALS  28
#define NSIG        NUMSIGNALS

typedef unsigned long sigset_t;

#define S_MASK(sig) (1<<((sig)-1))
/*
 *sa_flags values: SA_STACK is not currently supported, but will allow the
 * usage of signal stacks by using the (now obsolete) sa_restorer field in
 * the sigaction structure as a stack pointer. This is now possible due to
 * the changes in signal handling. LBT 010493.
 * SA_INTERRUPT is a no-op, but left due to historical reasons. Use the
 * SA_RESTART flag to get restarting signals (which were the default long ago)
 */
#define SA_NOCLDSTOP    1
#define SA_STACK        0x08000000
#define SA_RESTART      0x10000000
#define SA_INTERRUPT    0x20000000
#define SA_NOMASK       0x40000000
#define SA_ONESHOT      0x80000000

#define SIG_BLOCK          0    /* for blocking signals */
#define SIG_UNBLOCK        1    /* for unblocking signals */
#define SIG_SETMASK        2    /* for setting the signal mask */

typedef void (*_signal_handler_t)();
struct sigaction {
    int         sa_flags;       /* Special flags to affect behavior of signal */
    sigset_t    sa_mask;        /* Additional set of signals to be blocked during execution of signal-catching function. */
    _signal_handler_t sa_handler;
};

typedef struct sigcontext_s {
    int32_t edi, esi, ebp, ebx, edx, ecx, eax;
    int32_t eip, uesp;
    int32_t signo, sig_mask; // old mask
} sigcontext_t;

#define SIG_DFL ((_signal_handler_t)0)  /* Default action */
#define SIG_IGN ((_signal_handler_t)1)  /* Ignore action */
#define SIG_ERR ((_signal_handler_t)-1) /* Error return */

#endif
