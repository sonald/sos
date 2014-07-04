ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    . = 0x08000000;

    .text :
    {
        *(.text)
        *(.rodata)
    }
        
    _etext = .;

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
    }
            
    _end = .;
}
