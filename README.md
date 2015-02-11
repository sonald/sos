sos
===
Sian's experimenting Operating System written in C++ 

TODOs
====
+ tty and io
+ complete vfs and disk driver & block layer
+ kernel threads
+ enough syscalls for sh

ISSUSE
====
problems about pmm
+ physical mm is not good enough, and it's impossible to do va->pa & pa->va convertion. 
+ userspace alloc will make some virtual address in kernel space can not be mapped (by adding KERNEL_VIRTUAL_BASE). which in turn may cause kernel has no mem to alloc.
