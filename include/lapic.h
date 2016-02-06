#ifndef _SOS_LAPIC_H
#define _SOS_LAPIC_H 

#include <types.h>

uint32_t lapic_read(int reg);
void lapic_write(int reg, uint32_t val);

void lapic_init();

#endif

