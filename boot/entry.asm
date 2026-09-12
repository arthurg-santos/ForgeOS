global _start
extern kernel_main

section .bss
align 4096
pml4_table: resb 4096
pdpt_table: resb 4096
pd_table:   resb 4096
stack_bottom: resb 16384
stack_top:

section .text
bits 32
_start:
    mov esp, stack_top

    ; Limpar tabelas de página
    mov edi, pml4_table
    xor eax, eax
    mov ecx, 1024
    rep stosd

    mov edi, pdpt_table
    mov ecx, 1024
    rep stosd

    mov edi, pd_table
    mov ecx, 1024
    rep stosd

    ; PML4[0] = pdpt_table | 3 (Presente, Gravável)
    mov eax, pdpt_table
    or eax, 3
    mov [pml4_table], eax

    ; PDPT[0] = pd_table | 3
    mov eax, pd_table
    or eax, 3
    mov [pdpt_table], eax

    ; PD[0..511] = Páginas de 2MB (Mapeia o primeiro 1GB)
    mov edi, pd_table
    mov eax, 0x83 ; Presente, Gravável, Huge (2MB)
    mov ecx, 512
.map_pd:
    mov [edi], eax
    add eax, 0x200000 ; Incrementa 2MB
    add edi, 8
    loop .map_pd

    ; Carregar PML4 no registrador CR3
    mov eax, pml4_table
    mov cr3, eax

    ; Habilitar PAE (CR4.PAE = 1)
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Habilitar Long Mode (EFER.LME = 1)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Habilitar Paging e Modo Protegido (CR0.PG = 1, CR0.PE = 1)
    mov eax, cr0
    or eax, (1 << 31) | (1 << 0)
    mov cr0, eax

    ; Carregar GDT de 64 bits
    lgdt [gdt64.pointer]

    ; Far jump para o segmento de código de 64 bits
    jmp gdt64.code_segment:long_mode_start

bits 64
long_mode_start:
    ; Recarregar registradores de segmento
    mov ax, gdt64.data_segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; ---------------------------------------------------------
    ; HABILITAR FPU E SSE COMPLETAMENTE
    ; - CR0.EM = 0  : sem emulação de x87
    ; - CR0.TS = 0  : sem "task switched" (se TS=1, FPU/SSE gera #NM)
    ; - CR0.MP = 1  : monitor coprocessor
    ; - CR0.OSFXSR = 1 : OS suporta FXSAVE/SSE (sem isso, XMM gera #UD)
    ; - CR4.OSXMMEXCPT = 1 : exceções #XM de SSE
    ; ---------------------------------------------------------
    mov rax, cr0
    and rax, ~(1 << 2)   ; EM  = 0
    and rax, ~(1 << 3)   ; TS  = 0
    or  rax, (1 << 1)    ; MP  = 1
    or  rax, (1 << 18)   ; OSFXSR = 1
    mov cr0, rax

    mov rax, cr4
    or  rax, (1 << 10)   ; OSXMMEXCPT = 1
    mov cr4, rax

    fninit               ; Estado inicial sanado da x87
    ; ---------------------------------------------------------

    ; Chamar o kernel C++
    call kernel_main

    ; Travar a CPU caso o kernel retorne
    cli
.hang:
    hlt
    jmp .hang

section .rodata
gdt64:
    dq 0 ; Entrada nula
.code_segment: equ $ - gdt64
    dq (1<<53) | (1<<47) | (1<<44) | (1<<43) | (1<<41) ; Segmento de Código 64-bit
.data_segment: equ $ - gdt64
    dq (1<<47) | (1<<44) | (1<<41) ; Segmento de Dados
.pointer:
    dw $ - gdt64 - 1
    dq gdt64
