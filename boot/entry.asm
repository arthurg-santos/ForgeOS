global _start
global multiboot2_info_addr
extern kernel_main

section .bss
align 4096
pml4_table: resb 4096
pdpt_table: resb 4096
pd_table:   resb 4096
stack_bottom: resb 16384
stack_top:
multiboot2_info_addr: resq 1

section .text
bits 32
_start:
    mov esp, stack_top

    ; Salvar o ponteiro do Multiboot2 info (EBX) e validar o magic (EAX).
    ; O GRUB entrega EAX=0x36D76289 e EBX=ponteiro para a estrutura de tags.
    cmp eax, 0x36D76289
    je .mb_ok
    xor ebx, ebx          ; magic inválido: informa "sem info" ao kernel
.mb_ok:
    mov [multiboot2_info_addr], ebx

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

    ; Habilitar FPU e SSE (ver Fase 2): EM=0, TS=0, MP=1, OSFXSR=1, OSXMMEXCPT=1
    mov rax, cr0
    and rax, ~(1 << 2)
    and rax, ~(1 << 3)
    or  rax, (1 << 1)
    or  rax, (1 << 18)
    mov cr0, rax

    mov rax, cr4
    or  rax, (1 << 10)
    mov cr4, rax

    fninit

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
