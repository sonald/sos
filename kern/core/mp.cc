#include <mp.h>
#include <common.h>
#include <memlayout.h>
#include <string.h>

/*
 * http://wiki.osdev.org/Memory_Map_(x86)
 */

static uint32_t mp_magic = 0x5F504D5F; /* _MP_ */
static uint32_t mp_cfg_magic = 0x504D4350; /* PCMP */

static mp_floating_pointer_structure_t* mp_fps = NULL;
static mp_configuration_table_t* mp_cfg = NULL;

mp_t mp;

static bool scan_mp_fps()
{
    // scan floating pointer structure
    // a. In the first kilobyte of Extended BIOS Data Area (EBDA), or
    // b. Within the last kilobyte of system base memory (e.g., 639K-640K
    // for systems with 640KB of base memory or 511K-512K for systems with
    // 512 KB of base memory) if the EBDA segment is undefined, or
    // 	c. In the BIOS ROM address space between 0F0000h and 0FFFFFh.

    int step = 0;
    uint32_t base_ptr = *(uint16_t*)p2v(0x40E) << 4;
    uint32_t* EBDA_base = (uint32_t*)p2v(base_ptr);
    if (EBDA_base == 0) {
        EBDA_base = (uint32_t*)p2v(0x9FC00);
    }
    uint32_t ebda_size = *(uint16_t*)EBDA_base * 256;

    while (step < 3) {
        kprintf("search base: 0x%x, size 0x%x\n", EBDA_base, ebda_size);

        for (uint32_t i = 0; i < ebda_size; i++) {
            if (*(EBDA_base + i) == mp_magic) {
                mp_fps = (mp_floating_pointer_structure_t*)(EBDA_base + i);
                kprintf("found MP floating pointer structure at %x\n", mp_fps);
                step = 2;
                break;
            }
        }

        step++;
        if (!mp_fps && step == 0) {
            if (base_ptr != 0x9FC00) {
                EBDA_base = (uint32_t*)p2v(0x9FC00);
                continue;
            }
        }

        if (!mp_fps && step == 1) {
            EBDA_base = (uint32_t*)p2v(0xF0000);
            ebda_size = 1<<14;
        }
    }
    
    return (mp_fps != NULL);
}

void init_mp()
{

    if (!scan_mp_fps()) return;

    memset(&mp, 0, sizeof mp);

    kprintf("MPFPS: mp config 0x%x, len: %d, ver: 0x%x, IMCR present %d, default cfg: %d\n",
            mp_fps->configuration_table, (int)mp_fps->length * 16, mp_fps->mp_spec_rev,
            IMCR_PRESENT(mp_fps), mp_fps->default_configuration);

    mp.lapic_base = mp_cfg->lapic_address;

    if (mp_fps->configuration_table) {
        kassert(mp_fps->default_configuration == 0);
        mp_cfg = (mp_configuration_table_t*)p2v(mp_fps->configuration_table);

        kassert(mp_cfg->signature == mp_cfg_magic);
        char oem[9];
        char prod[13];
        strncpy(oem, mp_cfg->oem_id, 8);
        oem[8] = 0;
        strncpy(prod, mp_cfg->product_id, 12);
        prod[12] = 0;

        kprintf("MP CFG: len: %d, ver: 0x%x, oem: %s, prod id: %s\n"
                "entries: %d, lapic addr: 0x%x\n",
            mp_cfg->length, mp_cfg->mp_spec_rev, oem, prod, 
            mp_cfg->entry_count, mp_cfg->lapic_address);

        char* base = (char*)mp_cfg + sizeof(*mp_cfg);
        for (int i = 0; i < mp_cfg->entry_count; i++) {
            uint8_t type = *base;
            if (type == MP_CFG_ENTRY_PROCESSOR) {
                mp_configuration_entry_processor_t* p = 
                    (mp_configuration_entry_processor_t*)base;

                base += sizeof(mp_configuration_entry_processor_t);

                kprintf("Processor(%s): local apic id %d\n",
                        CPU_IS_BSP(p) ? "BSP": "AP", p->local_apic_id);

                auto& cpu = mp.cpus[mp.cpu_count++];
                cpu.cpu_id = p->local_apic_id;
                cpu.is_bsp = !!CPU_IS_BSP(p);
                cpu.lapic_addr = mp_cfg->lapic_address;
                if (cpu.is_bsp) {
                    mp.bsp = mp.cpu_count - 1;
                }
                
            } else if (type == MP_CFG_ENTRY_IO_APIC) {
                mp_configuration_entry_io_apic_t* apic = 
                    (mp_configuration_entry_io_apic_t*)base;
                base += sizeof(mp_configuration_entry_io_apic_t);

                kprintf("IOAPIC: id %d, address: 0x%x\n", 
                        apic->id, apic->address);
                mp.ioapic_base = apic->address;
                mp.ioapic_id = apic->id;
            } else {
                base += 8;
            }
        }
    }
}

