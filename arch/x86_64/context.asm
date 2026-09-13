bits 64

global switch_context

; void switch_context(uint64_t* old_rsp, uint64_t new_rsp)
; Salva os registradores callee-saved do contexto atual em old_rsp,
; carrega new_rsp e restaura o contexto de destino. O `ret` final
; salta para o ponto onde o destino parou (ou para a entry da task).
switch_context:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov [rdi], rsp
    mov rsp, rsi
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
