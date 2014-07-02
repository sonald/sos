#ifndef _MEM_LAYOUT_H
#define _MEM_LAYOUT_H 

#include "types.h"
// Memory layout

#define EXTMEM  0x100000            // Start of extended memory
#define PHYSTOP 0x4000000           // physical memory kernel can use at most
#define DEVSPACE 0xFE000000         // Other devices are at high addresses

#define USTACK  0xB0000000
#define UCODE   0x08000000

// Key addresses for address space layout (see kmap in vm.c for layout)
#define KERNEL_VIRTUAL_BASE 0xC0000000         // First kernel virtual address
#define KERNLINK (KERNEL_VIRTUAL_BASE+EXTMEM)  // Address where kernel is linked

#ifndef __ASSEMBLER__
static inline u32 v2p(void *a) { return ((u32)a)  - KERNEL_VIRTUAL_BASE; }
static inline void *p2v(u32 a) { return (void*)(a + KERNEL_VIRTUAL_BASE); }
#endif

#define V2P(a) (((u32)(a)) - KERNEL_VIRTUAL_BASE)
#define P2V(a) (((void*)(a)) + KERNEL_VIRTUAL_BASE)

#define V2P_WO(x) ((x) - KERNEL_VIRTUAL_BASE)    // same as V2P, but without casts
#define P2V_WO(x) ((x) + KERNEL_VIRTUAL_BASE)    // same as V2P, but without casts

#endif
