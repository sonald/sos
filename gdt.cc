#include "gdt.h"
#include "common.h"

BEGIN_CDECL

// Lets us access our ASM functions from our C code.
void gdt_flush(u32);
void idt_flush(u32);

void isr0();
void isr1();
void isr2();
void isr3();
void isr4();
void isr5();
void isr6();
void isr7();
void isr8();
void isr9();
void isr10();
void isr11();
void isr12();
void isr13();
void isr14();
void isr15();
void isr16();
void isr17();
void isr18();
void isr19();
void isr20();
void isr21();
void isr22();
void isr23();
void isr24();
void isr25();
void isr26();
void isr27();
void isr28();
void isr29();
void isr30();
void isr31();

void irq0();
void irq1();
void irq2();
void irq3();
void irq4();
void irq5();
void irq6();
void irq7();
void irq8();
void irq9();
void irq10();
void irq11();
void irq12();
void irq13();
void irq14();
void irq15();

END_CDECL

static void setup_gdt_entry(int idx, u32 base, u32 limit, u16 mode);
static void setup_idt_entry(int idx, u32 base, u16 selector, u8 flags);

// may need TSS entry later
gdt_entry_t gdt_entries[5];
gdt_ptr_t   gdt_ptr;
idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

void init_gdt()
{
    gdt_ptr.limit = sizeof(gdt_entries) - 1;
    gdt_ptr.base = (u32)&gdt_entries; // linear base of GDT

    setup_gdt_entry(0, 0, 0, 0);
    setup_gdt_entry(1, 0, 0xffffffff, GDT_CODE_PL0);
    setup_gdt_entry(2, 0, 0xffffffff, GDT_DATA_PL0);
    setup_gdt_entry(3, 0, 0xffffffff, GDT_CODE_PL3);
    setup_gdt_entry(4, 0, 0xffffffff, GDT_DATA_PL3);

    gdt_flush((u32)&gdt_ptr);
}

#define SETUP_GATE(i) \
    setup_idt_entry(i, (u32)isr##i, 0x08, GDTE_PRESENT(1) | GDTE_DPL(0) | IDTE_INT_GATE)

#define SETUP_IRQ(i, j) \
    setup_idt_entry(j, (u32)irq##i, 0x08, GDTE_PRESENT(1) | GDTE_DPL(0) | IDTE_INT_GATE)

void init_idt()
{
    // reprogram PICs
    // Remap the irq table.
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);

    idt_ptr.limit = sizeof(idt_entries) - 1;
    idt_ptr.base = (u32)&idt_entries;

    memset(&idt_entries, 0, sizeof(idt_entries));

    SETUP_GATE(0);
    SETUP_GATE(1);
    SETUP_GATE(2);
    SETUP_GATE(3);
    SETUP_GATE(4);
    SETUP_GATE(5);
    SETUP_GATE(6);
    SETUP_GATE(7);
    SETUP_GATE(8);
    SETUP_GATE(9);
    SETUP_GATE(10);
    SETUP_GATE(11);
    SETUP_GATE(12);
    SETUP_GATE(13);
    SETUP_GATE(14);
    SETUP_GATE(15);
    SETUP_GATE(16);
    SETUP_GATE(17);
    SETUP_GATE(18);
    SETUP_GATE(19);
    SETUP_GATE(20);
    SETUP_GATE(21);
    SETUP_GATE(22);
    SETUP_GATE(23);
    SETUP_GATE(24);
    SETUP_GATE(26);
    SETUP_GATE(27);
    SETUP_GATE(28);
    SETUP_GATE(29);
    SETUP_GATE(30);
    SETUP_GATE(31);

    SETUP_IRQ(0, 32);
    SETUP_IRQ(1, 33);
    SETUP_IRQ(2, 34);
    SETUP_IRQ(3, 35);
    SETUP_IRQ(4, 36);
    SETUP_IRQ(5, 37);
    SETUP_IRQ(6, 38);
    SETUP_IRQ(7, 39);

    SETUP_IRQ(8, 40);
    SETUP_IRQ(9, 41);
    SETUP_IRQ(10, 42);
    SETUP_IRQ(11, 43);
    SETUP_IRQ(12, 44);
    SETUP_IRQ(13, 45);
    SETUP_IRQ(14, 46);
    SETUP_IRQ(15, 47);

    idt_flush((u32)&idt_ptr);
}

#undef SETUP_GATE

void setup_gdt_entry(int idx, u32 base, u32 limit, u16 mode)
{
    gdt_entry_t* p = &gdt_entries[idx];
    p->limit_low = (limit & 0xffff);
    p->base_low = (base & 0xffff);
    p->base_middle = (base >>16) & 0xff;
    p->access = mode & 0xff;
    p->granularity = ((mode >> 8) & 0xf0) | ((limit >> 16) & 0x0f);
    p->base_high = (base >> 24) & 0xff;
}

void setup_idt_entry(int idx, u32 base, u16 selector, u8 flags)
{
    idt_entry_t *p = &idt_entries[idx];
    p->base_lo = (base & 0xffff);
    p->always0 = 0;
    p->flags = flags;
    p->sel = selector;
    p->base_hi = (base >> 16) & 0xffff;
}
