[BITS 32]
global switch_to_usermode
global flush_tss

[section .text]
; parameters: esp, eip
switch_to_usermode:
    push ebp
    mov ebp, esp

    cli

    mov ax, 0x23  ; GDT entry 4, RPL = 3
    mov ds, ax
    mov gs, ax
    mov es, ax
    mov fs, ax

    push 0x23 ;; ss
    mov eax, [ebp + 8]
    push eax ;; ring3 esp

    pushfd
    pop eax
    xor eax, 0x200 ;; enable IF
    push eax
    
    push 0x1b  ; GDT entry 3, RPL = 3
    push dword [ebp + 12] ;; eip
    iretd

    cli
    hlt ;; never return here

    mov esp, ebp
    pop ebp
    ret


flush_tss:
    mov ax, 0x28
    ltr ax
    ret
