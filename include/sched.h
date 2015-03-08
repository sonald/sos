#ifndef _SOS_SCHED_H
#define _SOS_SCHED_H 
#include "isr.h"

extern "C" void yield(trapframe_t* regs);
extern void scheduler(trapframe_t* regs);

#endif
