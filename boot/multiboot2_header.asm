section .multiboot2
align 8
multiboot2_header_start:
    dd 0xE85250D6                ; Magic number Multiboot2
    dd 0                         ; Arquitetura 0 (Modo Protegido i386 de 32 bits)
    dd multiboot2_header_end - multiboot2_header_start ; Tamanho do header
    dd 0x100000000 - (0xE85250D6 + 0 + (multiboot2_header_end - multiboot2_header_start)) ; Checksum

    ; End tag (Obrigatório para encerrar o header)
    dw 0    ; type
    dw 0    ; flags
    dd 8    ; size
multiboot2_header_end:
