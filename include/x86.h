#ifndef _X86_H
#define _X86_H 

#include "types.h"

static inline void sti() { asm volatile ("sti":::"cc"); }
static inline void cli() { asm volatile ("cli":::"cc"); }

// port io
static inline void outb(u16 port, u8 val)
{
    __asm__ __volatile__ ( "outb %1, %0" : : "dN"(port), "a"(val));
}

static inline u8 inb(u16 port)
{
    u8 val;
    __asm__ __volatile__ ( "inb %1, %0" : "=a"(val) : "dN"(port));
    return val;
}

static inline u16 inw(u16 port)
{
    u16 val;
    __asm__ __volatile__ ( "inw %1, %0" : "=a"(val) : "dN"(port));
    return val;
}

// Eflags register
#define FL_CF           0x00000001      // Carry Flag
#define FL_PF           0x00000004      // Parity Flag
#define FL_AF           0x00000010      // Auxiliary carry Flag
#define FL_ZF           0x00000040      // Zero Flag
#define FL_SF           0x00000080      // Sign Flag
#define FL_TF           0x00000100      // Trap Flag
#define FL_IF           0x00000200      // Interrupt Enable
#define FL_DF           0x00000400      // Direction Flag
#define FL_OF           0x00000800      // Overflow Flag
#define FL_IOPL_MASK    0x00003000      // I/O Privilege Level bitmask
#define FL_IOPL_0       0x00000000      //   IOPL == 0
#define FL_IOPL_1       0x00001000      //   IOPL == 1
#define FL_IOPL_2       0x00002000      //   IOPL == 2
#define FL_IOPL_3       0x00003000      //   IOPL == 3
#define FL_NT           0x00004000      // Nested Task
#define FL_RF           0x00010000      // Resume Flag
#define FL_VM           0x00020000      // Virtual 8086 mode
#define FL_AC           0x00040000      // Alignment Check
#define FL_VIF          0x00080000      // Virtual Interrupt Flag
#define FL_VIP          0x00100000      // Virtual Interrupt Pending
#define FL_ID           0x00200000      // ID flag

static inline uint32_t readflags() {
    uint32_t old;
    asm volatile ("pushfl; popl %0":"=r"(old)::"memory");
    return old;
}

static inline void writeflags(uint32_t flags) {
    asm volatile ("pushl %0; popfl"::"r"(flags):"memory", "cc");
}

static inline int bts(uint32_t* val) {
    int ret = 0;
    asm volatile ("lock bts $0, %1 \n"
            "jnc 1f \n"
            "movl $1, %0 \n"
            "1: "
            :"=m"(ret):"m"(*val) :"memory", "cc");
    return ret;
}

#endif
