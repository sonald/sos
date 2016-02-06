#ifndef _SOS_MP_H
#define _SOS_MP_H

#include <types.h>

#define IMCR_P_BIT 0x80

#define IMCR_PRESENT(mpfps)  ((mpfps)->features1 & IMCR_P_BIT)

#define PIC_MODE_IMPL(mpfps) (IMCR_PRESENT(mpfps))
#define VIRTUAL_WIRE_MODE_IMPL(mpfps) (!IMCR_PRESENT(mpfps))

typedef struct mp_floating_pointer_structure_s
{
    uint32_t magic; // _MP_
    uint32_t configuration_table;
    uint8_t length; // In 16 bytes (e.g. 1 = 16 bytes, 2 = 32 bytes)
    uint8_t mp_spec_rev;
    uint8_t checksum;
    uint8_t default_configuration;
    uint8_t features1;
    uint16_t features_reserved;

} __attribute__((packed)) mp_floating_pointer_structure_t;

typedef struct mp_configuration_table_s 
{
    uint32_t signature; // "PCMP"
    uint16_t length;
    uint8_t mp_spec_rev;
    uint8_t checksum; // Again, the byte should be all bytes in the table add up to 0
    char oem_id[8];
    char product_id[12];
    uint32_t oem_table;
    uint16_t oem_table_size;
    uint16_t entry_count; // This value represents how many entries are following this table
    uint32_t lapic_address; // This is the memory mapped address of the local APICs
    uint16_t extended_table_length;
    uint8_t extended_table_checksum;
    uint8_t reserved;
} __attribute__((packed)) mp_configuration_table_t;


/*
 * base entry types
 */
enum {
    MP_CFG_ENTRY_PROCESSOR          = 0,
    MP_CFG_ENTRY_BUS                = 1,
    MP_CFG_ENTRY_IO_APIC            = 2,
    MP_CFG_ENTRY_IO_INTR            = 3,
    MP_CFG_ENTRY_LOCAL_INTR         = 4,
};

#define CPU_ENABLED(entry) ((entry)->flags & 0x01)
#define CPU_IS_BSP(entry) ((entry)->flags & 0x02)

typedef struct mp_configuration_entry_processor_s 
{
    uint8_t type; // Always 0
    uint8_t local_apic_id;
    uint8_t local_apic_version;
    uint8_t flags; // If bit 0 is clear then the processor must be ignored
                   // If bit 1 is set then the processor is the BSP(bootstrap processor)
    uint32_t signature;
    uint32_t feature_flags;
    uint8_t reserved[8];
} __attribute__((packed)) mp_configuration_entry_processor_t;

#define IOAPIC_ENABLED(entry) ((entry)->flags & 0x01)

typedef struct mp_configuration_entry_io_apic_s 
{
    uint8_t type; // Always 2
    uint8_t id;
    uint8_t version;
    uint8_t flags; // If bit 0 is set then the entry should be ignored
    uint32_t address; // The memory mapped address of the IO APIC is memory
} __attribute__((packed)) mp_configuration_entry_io_apic_t;

/* this is processor's local address space */
#define LAPIC_DEFAULT_ADDR 0xFEE00000
/* this is global for all processors to share */
#define IOAPIC_DEFAULT_ADDR 0xFEC00000

typedef struct cpu_s 
{
    int cpu_id;
    uint32_t lapic_addr;
    bool is_bsp;
} cpu_t;

#define MAX_NR_CPU 4

typedef struct mp_s 
{
    cpu_t cpus[MAX_NR_CPU];
    int cpu_count;
    int bsp;
    uint32_t lapic_base;

    // this assumes only one io apic exists
    uint32_t ioapic_base;
    int ioapic_id;
} mp_t;

extern mp_t mp;
extern void init_mp();

#endif // _SOS_MP_H
