sos
===
Sian's experimenting Operating System written in C++ 

TODOs
====
+ tty and io
+ complete vfs and disk driver & block layer
+ kernel threads (share kernel mapping, do not reload cr3)
+ enough syscalls for sh

ISSUSE
====
problems about pmm
+ physical mm is not good enough, and it's impossible to do va->pa & pa->va convertion. 
+ userspace alloc will make some virtual address in kernel space can not be mapped (by adding KERNEL_VIRTUAL_BASE). which in turn may cause kernel has no mem to alloc.
+ xv6 use trap gate for syscalls, which leaves IF as it was(not cleared).
+ spinlock can not used in irq context in uniprocess system.
+ take care of the IF state, sleep can only happen with IF enabled context 
  and wakeup from irq context leave IF disabled.
+ about spinlock: busy-waiting isn't enough when access syscall is a trap gate (IF enabled).
  we need to make sure cli'ed so fork / exec will not be preempted before finished.
