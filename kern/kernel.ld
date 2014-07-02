ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    . = 0xC0100000;

    .text 0xC0100000: AT(ADDR(.text) - 0xC0000000)
    {
        *(.mboot)
        *(.text)
        *(.rodata)
    }
        
    . = ALIGN(0x1000);

    _data = .;

    .data : 
    { 
        *(.data)
    }
        
    .bss :
    {
        *(COMMON)
        *(.bss)
        *(.bootstrap_stack)
    }
            
    _end = .;
}
