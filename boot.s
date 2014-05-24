; This is the kernel's entry point. We could either call main here,
; or we can use this to setup the stack or other nice stuff, like
; perhaps setting up the GDT and segments. Please note that interrupts
; are disabled at this point: More on interrupts later!
[BITS 32]
global start
extern kernel_main
extern kernel_init
extern __cxa_finalize
extern _init
extern _fini

; setup higher-half kernel
; temporary Page Dir (4KB pages)
KERNEL_VIRTUAL_BASE equ 0xC0000000
KERNEL_PDE_INDEX equ (KERNEL_VIRTUAL_BASE >> 22)
[section .data]
ALIGN 0x1000
KERNEL_PAGE_ATTR equ  0x03 ; P, RW, 4K page
boot_kernel_pagetable:
    dd (0 | KERNEL_PAGE_ATTR)
    times (KERNEL_PDE_INDEX - 1) dd 0
    dd (0 | KERNEL_PAGE_ATTR)
    times (1024 - KERNEL_PDE_INDEX - 1) dd 0

; seems qemu does not support PSE
; temporary Page Dir (4MB pages)
[section .data]
ALIGN 0x1000
KERNEL_4M_PAGE_ATTR equ  0x83 ; P, RW, PSE(4M Page)
boot_kernel_pagetable_4m:
    dd (0 | KERNEL_4M_PAGE_ATTR)
    times (KERNEL_PDE_INDEX - 1) dd 0
    dd (0 | KERNEL_4M_PAGE_ATTR)
    times (1024 - KERNEL_PDE_INDEX - 1) dd 0

[section .data]
ALIGN 0x1000
table001:
    times 1024 dd 0
table768:
    times 1024 dd 0

[section .mboot]
; This part MUST be 4byte aligned, so we solve that issue using 'ALIGN 4'
ALIGN 4
mboot:
    ; Multiboot macros to make a few lines later more readable
    MULTIBOOT_PAGE_ALIGN    equ 1<<0
    MULTIBOOT_MEMORY_INFO   equ 1<<1
    MULTIBOOT_HEADER_MAGIC  equ 0x1BADB002
    MULTIBOOT_HEADER_FLAGS  equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO 
    MULTIBOOT_CHECKSUM  equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

    ; This is the GRUB Multiboot header. A boot signature
    dd MULTIBOOT_HEADER_MAGIC
    dd MULTIBOOT_HEADER_FLAGS
    dd MULTIBOOT_CHECKSUM
    
%macro fill_page_table 2
    mov eax, boot_kernel_pagetable - KERNEL_VIRTUAL_BASE
    add eax, %2

    or dword [eax], %1 - KERNEL_VIRTUAL_BASE
    mov ecx, 1024
    mov eax, 0x03 ; RW, P
    mov edi, (%1 - KERNEL_VIRTUAL_BASE)
.%1.1:
    stosd
    add eax, 4096
    loop .%1.1
%endmacro

[section .text]
start equ (_start - KERNEL_VIRTUAL_BASE)
_start:
    ; we really should check cpuid for PSE support, when support, use 4MB page
    ;mov eax, 01
    ;cpuid
    ;and edx, 0x00000004 ; check PSE
    ;jz _panic

    ; populate table001, table768
    fill_page_table table001, 0
    fill_page_table table768, 768<<2

    ;mov eax, boot_kernel_pagetable - KERNEL_VIRTUAL_BASE
    ;or dword [eax], table001 - KERNEL_VIRTUAL_BASE
    ;mov ecx, 1024
    ;mov eax, 0x03 ; RW, P
    ;mov edi, (table001 - KERNEL_VIRTUAL_BASE)
;.1:
    ;stosd
    ;add eax, 4096
    ;loop .1

    ;mov eax, boot_kernel_pagetable - KERNEL_VIRTUAL_BASE
    ;add eax, 768<<2
    ;or dword [eax], table768 - KERNEL_VIRTUAL_BASE
    ;mov ecx, 1024
    ;mov eax, 0x03 ; RW, P
    ;mov edi, (table768 - KERNEL_VIRTUAL_BASE)
;.2:
    ;stosd
    ;add eax, 4096
    ;loop .2

    ; enable PSE
    ;mov ecx, cr4
    ;or ecx, 0x00000010 
    ;mov cr4, ecx

    mov ecx, (boot_kernel_pagetable - KERNEL_VIRTUAL_BASE)
    mov cr3, ecx

    mov ecx, cr0
    or ecx, 0x80000000 ; PG=1
    mov cr0, ecx

    ; an absolute jump into virtual address of next instruction
    lea ecx, [higher_half_entry]
    jmp ecx

higher_half_entry:
    ; invalidate pde 0
    ;mov dword [boot_kernel_pagetable], 0x0
    ;invlpg [0]

    mov esp, kern_stack_top
    
    call kernel_init 
    call _init

    ; cdecl, ebx will keep as it was
    push ebx

    call kernel_main
    call _fini

    sub esp, 4
    push 0
    call __cxa_finalize

    cli
    hlt
_panic:
    jmp $

[section .bootstrap_stack nobits]
_kern_stack:
    resb 8192
kern_stack_top:

