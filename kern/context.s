[section .text]
[BITS 32]

global switch_to_usermode
global flush_tss
global yield
global switch_to

extern trap_return
extern scheduler

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

yield:
    ret

;; [esp] old ret eip
;; [esp+4] old addr of kctx
;; [esp+8] next kctx
switch_to:
    mov eax, [esp+4]
    mov edx, [esp+8] 

    ;; eip is right at esp, no need to push
    push ebp
    push edi
    push esi
    push ebx

    mov [eax], esp ;; save kctx at old
    mov esp, edx 

    pop ebx
    pop esi
    pop edi
    pop ebp
    ;; this will return to trap_return if new task is not run before.
    ;; else, return to break point in isr
    ret
