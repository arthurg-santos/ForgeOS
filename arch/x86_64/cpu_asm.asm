bits 64

global gdt_flush
global tss_flush

; void gdt_flush(uint64_t gdt_ptr_addr)
; Carrega a GDT e recarrega todos os segmentos de forma atômica.
gdt_flush:
    lgdt [rdi]

    ; Recarrega segmentos de dados (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Recarrega CS (0x08) via far return
    push 0x08
    lea rax, [rel .done]
    push rax
    retfq
.done:
    ret

; void tss_flush(uint16_t selector)
tss_flush:
    mov ax, di
    ltr ax
    ret
