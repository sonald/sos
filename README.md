![logo](images/logo.ppm?raw=true)

sos
===
Sian's experimenting Operating System written in C++. it's a 32bit os kernel and runs on uniprocessor.

![screenshot](images/screenshot.png?raw=true)

BUILD
====
Like `makefile` indicated, this is a x86 kernel, and you need a i686 gcc cross compiler for building and running it. see [here](http://wiki.osdev.org/GCC_Cross-Compiler) for the information. I think it also works under a 32bit GNU/Linux distribution. you may need to specify `-Wl,--build-id=none` for successful compiling. The kernel follows multiboot protocol and need a hd img with grub2 preinstalled. [Here](http://pan.baidu.com/s/1o6uZ1s2) is a premade image for testing.

Refs
===
[xv6][] is a great reference implementation of a unix style kernel. [linux 0.11][linux] is also worth reading, and I learned a lot from these projects. You can find a lot of info about writing an small os kernel [here](http://wiki.osdev.org).

Notes
===
One of the hardest as far as I can tell is memory management.    I do implement my own kmalloc/kfree based on a simple algorithm. The way I choose to manage memory (both physical and virtual) makes some thing hard to implement, i.e shared memory region. 

Another thing is task switch. There are many ways to do this,
[xv6][]'s way or old linux way or your own method.

When design other parts of the kernel, you can stick to the unix tradition or just go bold and invent your own.


TODOs
====
+ tty and io partially works
+ enough syscalls for sh
+ write support of vfs & block io layers
+ signal handling (partially)
+ simple GUI and software renderer (may port mesa if possible)
+ port newlib as userland libc

ISSUSE
====
+ physical mm is not good enough, and it's impossible to do va->pa & pa->va convertion.
+ userspace alloc will make some virtual address in kernel space can not be mapped (by adding KERNEL_VIRTUAL_BASE). which in turn may cause kernel has no mem to alloc.
+ sos use trap gate for syscalls, which leaves IF as it was(not cleared). must be very careful.
+ spinlock can not used in irq context in uniprocess system.
+ take care of the IF state, sleep can only happen with IF enabled context
  and wakeup from irq context leave IF disabled.
+ about spinlock: busy-waiting isn't enough when access syscall is a trap gate (IF enabled).
  we need to make sure cli'ed so fork / exec will not be preempted before finished.


[xv6]: http://pdos.csail.mit.edu/6.828/2014/xv6.html
[linux]: http://oldlinux.org
