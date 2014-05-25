#ifndef _GDT_H
#define _GDT_H 

#include "common.h"

// This structure contains the value of one GDT entry.
// We use the attribute 'packed' to tell GCC not to change
// any of the alignment in the structure.
struct gdt_entry_ {
   u16 limit_low;           // The lower 16 bits of the limit.
   u16 base_low;            // The lower 16 bits of the base.
   u8  base_middle;         // The next 8 bits of the base.
   u8  access;              // Access flags, determine what ring this segment can be used in.
   u8  granularity;
   u8  base_high;           // The last 8 bits of the base.
} __attribute__((packed));
typedef struct gdt_entry_ gdt_entry_t;

struct gdt_ptr_
{
   u16 limit;               // The upper 16 bits of all selector limits.
   u32 base;                // The address of the first gdt_entry_t struct.
} __attribute__((packed));
typedef struct gdt_ptr_ gdt_ptr_t;

//access
#define GDTE_DT_CD      (1 << 4) // code or data seg type
#define GDTE_DT_GATE    (0 << 4) // system or gate type
#define GDTE_PRESENT(x) ((x) << 7) // present
#define GDTE_DPL(x)     ((x & 0x03) << 5) // DPL

// granularity
#define GDTE_G(x)       ((x) << 0xf)  // 0 for byte, 1 for 4KB
#define GDTE_SIZE(x)    ((x) << 0xe) // 1 for 32bit addr or 4GB bound

// segment type
#define SEG_DATA_R        0x00 // Read-Only
#define SEG_DATA_RA       0x01 // Read-Only, accessed
#define SEG_DATA_RW       0x02 // Read/Write
#define SEG_DATA_RWA      0x03 // Read/Write, accessed
#define SEG_DATA_REXPD    0x04 // Read-Only, expand-down
#define SEG_DATA_REXPDA   0x05 // Read-Only, expand-down, accessed
#define SEG_DATA_RWEXPD   0x06 // Read/Write, expand-down
#define SEG_DATA_RWEXPDA  0x07 // Read/Write, expand-down, accessed
#define SEG_CODE_E        0x08 // Execute-Only
#define SEG_CODE_EA       0x09 // Execute-Only, accessed
#define SEG_CODE_ER       0x0A // Execute/Read
#define SEG_CODE_ERA      0x0B // Execute/Read, accessed
#define SEG_CODE_EC       0x0C // Execute-Only, conforming
#define SEG_CODE_ECA      0x0D // Execute-Only, conforming, accessed
#define SEG_CODE_ERC      0x0E // Execute/Read, conforming
#define SEG_CODE_ERCA     0x0F // Execute/Read, conforming, accessed
 

#define GDT_CODE_PL0 GDTE_DT_CD | GDTE_PRESENT(1) | GDTE_SIZE(1) | GDTE_G(1) | \
    SEG_CODE_ER | GDTE_DPL(0) 

#define GDT_DATA_PL0 GDTE_DT_CD | GDTE_PRESENT(1) | GDTE_SIZE(1) | GDTE_G(1) | \
    SEG_DATA_RW | GDTE_DPL(0) 

#define GDT_CODE_PL3 GDTE_DT_CD | GDTE_PRESENT(1) | GDTE_SIZE(1) | GDTE_G(1) | \
    SEG_CODE_ER | GDTE_DPL(3) 

#define GDT_DATA_PL3 GDTE_DT_CD | GDTE_PRESENT(1) | GDTE_SIZE(1) | GDTE_G(1) | \
    SEG_DATA_RW | GDTE_DPL(3) 


// A struct describing an interrupt gate.
struct idt_entry_s
{
   u16 base_lo;             // The lower 16 bits of the address to jump to when this interrupt fires.
   u16 sel;                 // Kernel segment selector.
   u8 always0;             // This must always be zero.
   u8 flags;               // More flags. See documentation.
   u16 base_hi;             // The upper 16 bits of the address to jump to.
} __attribute__((packed));
typedef struct idt_entry_s idt_entry_t;

// A struct describing a pointer to an array of interrupt handlers.
// This is in a format suitable for giving to 'lidt'.
struct idt_ptr_s
{
   u16 limit;
   u32 base;                // The address of the first element in our idt_entry_t array.
} __attribute__((packed));
typedef struct idt_ptr_s idt_ptr_t;

//int gate will reset IF flag, trap gate will not
#define IDTE_INT_GATE  0x0E
#define IDTE_TRAP_GATE 0x0F

void init_gdt();
void init_idt();


#endif
