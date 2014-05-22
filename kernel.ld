ENTRY(start)
SECTIONS
{
    . = 1M;
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.mboot)
        *(.text)
        *(.rodata)
    }
        
    .data BLOCK(4K) : ALIGN(4K)
    { 
        *(.data)
    }
        
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
        *(.bootstrap_stack)
    }
            
}
