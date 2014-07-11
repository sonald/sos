[BITS 32]
global gdt_flush
global idt_flush
global trap_return

extern isr_handler
extern irq_handler

gdt_flush:
    mov eax, [esp+4]
    lgdt [eax]

    mov ax, 0x10
    mov ds, ax
    mov gs, ax
    mov fs, ax
    mov es, ax
    jmp 0x08:.flush ; jump to new code selector
.flush:
    ret

idt_flush:
    mov eax, [esp+4]
    lidt [eax]
    ret

%macro isr_noerrcode 1
global isr%1
isr%1:
    cli ; seems unneccesary due to int-gate
    push 0
    push %1
    jmp isr_stub
%endmacro

%macro isr_errcode 1
global isr%1
isr%1:
    cli ; seems unneccesary due to int-gate
    push %1
    jmp isr_stub
%endmacro

%macro def_irq 2
global irq%1
irq%1:
    cli ; seems unneccesary cause I use int-gate
    push 0
    push %2
    jmp irq_stub
%endmacro

%macro handler_stub 2
%1:
    pusha ; this will do pushad
    push ds
    push es
    push gs
    push fs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax

    push esp
    call %2
    add esp, 4

    jmp trap_return
%endmacro

trap_return:
    pop fs
    pop gs
    pop es
    pop ds
    popa
    add esp, 8
    iret

isr_noerrcode 0
isr_noerrcode 1
isr_noerrcode 2
isr_noerrcode 3
isr_noerrcode 4
isr_noerrcode 5
isr_noerrcode 6
isr_noerrcode 7
isr_errcode   8
isr_noerrcode 9
isr_errcode   10
isr_errcode   11
isr_errcode   12
isr_errcode   13
isr_errcode   14
isr_noerrcode 15
isr_noerrcode 16
isr_noerrcode 17
isr_noerrcode 18
isr_noerrcode 19
isr_noerrcode 20
isr_noerrcode 21
isr_noerrcode 22
isr_noerrcode 23
isr_noerrcode 24
isr_noerrcode 25
isr_noerrcode 26
isr_noerrcode 27
isr_noerrcode 28
isr_noerrcode 29
isr_noerrcode 30
isr_noerrcode 31

isr_noerrcode 128

handler_stub isr_stub, isr_handler

; master PIC
def_irq 0, 32
def_irq 1, 33
def_irq 2, 34
def_irq 3, 35
def_irq 4, 36
def_irq 5, 37
def_irq 6, 38
def_irq 7, 39
; slave PIC
def_irq 8, 40
def_irq 9, 41
def_irq 10, 42
def_irq 11, 43
def_irq 12, 44
def_irq 13, 45
def_irq 14, 46
def_irq 15, 47

handler_stub irq_stub, irq_handler
