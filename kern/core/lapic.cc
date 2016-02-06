#include <lapic.h>
#include <mp.h>
#include <isr.h>

/*
 * 0x0020h  Local APIC ID Register  Read/Write
 * 0x0030h  Local APIC ID Version Register  Read only
 * 0x0040h - 0x0070h    reserved    -
 * 0x0080h  Task Priority Register  Read/Write
 * 0x0090h  Arbitration Priority Register   Read only
 * 0x00A0h  Processor Priority Register Read only
 * 0x00B0h  EOI Register    Write only
 * 0x0090h  Arbitration Priority Register   Read only
 * 0x00A0h  Processor Priority Register Read only
 * 0x00B0h  EOI Register    Write only
 * 0x00C0h  reserved    -
 * 0x00D0h  Logical Destination Register    Read/Write
 * 0x00E0h  Destination Format Register Bits 0-27 Read only, Bits 28-31 Read/Write
 * 0x00F0h  Spurious-Interrupt Vector Register  Bits 0-3 Read only, Bits 4-9 Read/Write
 * 0x0100h - 0x0170 ISR 0-255   Read only
 * 0x0180h - 0x01F0h    TMR 0-255   Read only
 * 0x0200h - 0x0270h    IRR 0-255   Read only
 * 0x0280h  Error Status Register   Read only
 * 0x0290h - 0x02F0h    reserved    -
 * 0x0300h  Interrupt Command Register 0-31 Read/Write
 * 0x0310h  Interrupt Command Register 32-63    Read/Write
 * 0x0320h  Local Vector Table (Timer)  Read/Write
 * 0x0330h  reserved    -
 * 0x0340h  Performance Counter LVT Read/Write
 * 0x0350h  Local Vector Table (LINT0)  Read/Write
 * 0x0360h  Local Vector Table (LINT1)  Read/Write
 * 0x0370h  Local Vector Table (Error)  Read/Write
 * 0x0380h  Initial Count Register for Timer    Read/Write
 * 0x0390h  Current Count Register for Timer    Read only
 * 0x03A0h - 0x03D0h    reserved    -
 * 0x03E0h  Timer Divide Configuration Register Read/Write
 */
enum LAPIC_REG_T 
{
    LAPIC_ID = 0x20,
    LAPIC_ID_VERSION = 0x30,
    TASK_PRIORITY = 0x80,
    ARBITRATION_PRIORITY = 0x90,
    PROCESSOR_PRIORITY = 0xA0,
    EOI = 0xB0,
    LOGICAL_DESTINATION = 0xD0,
    DEST_FORMAT = 0xE0,
    SPURIOUS_INTR_VECTOR = 0xF0,
        
    ERROR_STATUS = 0x0280,
    ICR_LOW = 0x0300,
    ICR_HIGH = 0x0310,

    LVT_TIMER = 0x0320,
    LVT_PC = 0x0340,
    LINT0 = 0x0350,
    LINT1 = 0x0360,
    LVT_ERROR = 0x0370,
    TIMER_INIT_COUNT = 0x0380,
    TIMER_CURRENT_COUNT = 0x03A0,
    TIMER_DIVIDE_CONFIG = 0x03E0,
};

uint32_t lapic_read(int reg)
{
    volatile uint32_t* lapic = (uint32_t*)mp.lapic_base;
    return lapic[reg<<2];
}

void lapic_write(int reg, uint32_t val)
{
    volatile uint32_t* lapic = (uint32_t*)mp.lapic_base;
    lapic[reg<<2] = val;
}

void enable_lapic()
{
    uint32_t val = lapic_read(SPURIOUS_INTR_VECTOR);
    val |= 0x0100;
    lapic_write(SPURIOUS_INTR_VECTOR, val);
}

void lapic_init()
{
    lapic_write(SPURIOUS_INTR_VECTOR, 0x0100 | IRQ_SPURIOUS);
    lapic_write(LVT_ERROR, IRQ_ERROR);
}
