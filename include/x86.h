#ifndef _X86_H
#define _X86_H 

inline void sti() { asm volatile ("sti":::"cc"); }
inline void cli() { asm volatile ("cli":::"cc"); }

#endif
