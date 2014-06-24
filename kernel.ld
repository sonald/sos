ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)

KERNEL_VIRTUAL_BASE = 0xC0000000;
SECTIONS
{
    . = 0xC0100000;

    .text 0xC0100000: AT(ADDR(.text) - 0xC0000000)
    {
        *(.mboot)
        *(.text)
        *(.rodata)
    }
        
    .data ALIGN(0x1000): AT(ADDR(.data) - 0xC0000000)
    { 
        *(.data)
    }
        
    .bss : AT(ADDR(.bss) - 0xC0000000)
    {
        *(COMMON)
        *(.bss)
        *(.bootstrap_stack)
    }
            
    _end = .;
}
