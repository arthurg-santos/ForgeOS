Excelente iniciativa. Construir um sistema operacional do zero é uma das jornadas mais desafiadoras e recompensadoras da engenharia de software. O **ForgeOS** será projetado com uma arquitetura limpa, modular e preparada para escalar, respeitando rigorosamente as suas restrições de ambiente (Fedora 44, QEMU, x86_64) e a filosofia educacional.

Abaixo está o projeto completo, passo a passo, para a **Fase 0 e Fase 1** do ForgeOS.

---

### 1. Visão geral do ForgeOS

O **ForgeOS** é um sistema operacional de 64 bits (x86_64) construído do zero com foco em clareza arquitetural e aprendizado profundo. O objetivo desta primeira versão funcional é estabelecer a fundação do sistema: realizar o boot através de um bootloader padrão, transitar a CPU do Modo Protegido (32 bits) para o Modo Longo (64 bits), configurar o ambiente C++ *freestanding* e exibir mensagens no console de texto VGA. Não há dependências de sistemas operacionais hospedeiros dentro da VM, garantindo que o código seja 100% bare-metal.

### 2. Decisões técnicas

*   **Bootloader (GRUB2 + Multiboot2):** O Multiboot2 é o padrão da indústria para carregar kernels x86_64 de forma agnóstica. O GRUB2 cuida do hardware inicial e nos entrega a CPU em Modo Protegido (32 bits). A transição para o Modo Longo (64 bits) é feita pelo nosso próprio código em Assembly, o que é fundamental para entender como a CPU funciona.
*   **Toolchain (GCC/G++ Nativo do Fedora):** Para evitar compilações gigantes e desnecessárias de *cross-compilers*, utilizaremos o `gcc` e `g++` nativos do Fedora, mas com flags estritas (`-ffreestanding`, `-nostdlib`, `-mcmodel=kernel`, `-mno-red-zone`). Isso impede o linker de buscar a `libc` ou `libstdc++` do Linux, forçando um ambiente bare-metal real.
*   **Assembly (NASM):** Utilizado apenas onde o C++ não pode atuar: o cabeçalho Multiboot2, configuração de tabelas de página (Paging) e o salto para o Modo Longo.
*   **Console (VGA Text Mode):** O buffer de memória em `0xB8000` é a forma mais simples, rápida e didática de exibir texto sem precisar inicializar uma GPU complexa nesta fase inicial.
*   **Tipos de Dados Próprios:** Para garantir 100% de isolamento do sistema hospedeiro, não usaremos `<stdint.h>` ou `<stddef.h>` do sistema. Criaremos nosso próprio arquivo `types.h` com os tamanhos exatos garantidos pela arquitetura x86_64.

### 3. Arquitetura inicial

1.  **Camada de Boot (Assembly):** O GRUB carrega o kernel. O Assembly configura a stack, cria as tabelas de página (PML4, PDPT, PD) para mapear a memória, habilita o PAE e o Long Mode via registradores de controle (CR4, EFER, CR0), carrega uma GDT de 64 bits e faz um *far jump* para o código de 64 bits.
2.  **Camada de Kernel (C++):** O ponto de entrada `kernel_main` é chamado. Ele inicializa o namespace `Forge::Console`, limpa a tela e imprime as mensagens de boas-vindas.
3.  **Camada de Linker:** O script de linker garante que o cabeçalho Multiboot2 esteja no início do arquivo binário (exigência do GRUB) e alinha as seções de memória corretamente a partir do endereço físico `1MB`.

---

### 4. Árvore completa do projeto

```text
ForgeOS/
├── .gitignore
├── Makefile
├── README.md
├── boot/
│   ├── entry.asm
│   └── multiboot2_header.asm
├── include/
│   ├── io.h
│   └── types.h
├── kernel/
│   ├── io.cpp
│   └── main.cpp
├── linker/
│   └── linker.ld
└── scripts/
    └── grub.cfg
```

---

### 5. Código completo de cada arquivo

Abaixo está o código integral de todos os arquivos. Copie exatamente como está apresentado.

#### `ForgeOS/Makefile`

```makefile
# Toolchain
CC = gcc
CXX = g++
ASM = nasm
LD = gcc

# Flags de compilação C/C++ (Ambiente Freestanding, Sem bibliotecas padrão, Kernel 64-bit)
CFLAGS = -ffreestanding -fno-pie -fno-pic -mno-red-zone -mcmodel=kernel -nostdlib -fno-builtin -Wall -Wextra -O2 -g
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASMFLAGS = -f elf64
LDFLAGS = -ffreestanding -nostdlib -lgcc -no-pie -T linker/linker.ld

# Diretórios e Arquivos
BUILD_DIR = build
ISO_DIR = $(BUILD_DIR)/iso
KERNEL_ELF = $(BUILD_DIR)/forgeos.elf
ISO_NAME = $(BUILD_DIR)/forgeos.iso

ASM_SOURCES = boot/multiboot2_header.asm boot/entry.asm
CXX_SOURCES = kernel/main.cpp kernel/io.cpp

ASM_OBJECTS = $(ASM_SOURCES:%.asm=$(BUILD_DIR)/%.o)
CXX_OBJECTS = $(CXX_SOURCES:%.cpp=$(BUILD_DIR)/%.o)
OBJECTS = $(ASM_OBJECTS) $(CXX_OBJECTS)

.PHONY: all clean run debug dirs

all: dirs $(ISO_NAME)

dirs:
	@mkdir -p $(BUILD_DIR)/boot
	@mkdir -p $(BUILD_DIR)/kernel
	@mkdir -p $(ISO_DIR)/boot/grub

$(BUILD_DIR)/%.o: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -Iinclude

$(KERNEL_ELF): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

$(ISO_NAME): $(KERNEL_ELF)
	@cp $(KERNEL_ELF) $(ISO_DIR)/boot/forgeos.elf
	@cp scripts/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	@grub2-mkrescue -o $@ $(ISO_DIR) --modules="multiboot2" 2>/dev/null || grub-mkrescue -o $@ $(ISO_DIR) --modules="multiboot2" 2>/dev/null

run: all
	@qemu-system-x86_64 -cdrom $(ISO_NAME) -m 128M -vga std -serial stdio -no-reboot -no-shutdown

debug: all
	@qemu-system-x86_64 -cdrom $(ISO_NAME) -m 128M -vga std -serial stdio -no-reboot -no-shutdown -s -S &
	@gdb -ex "target remote :1234" -ex "symbol-file $(KERNEL_ELF)"

clean:
	@rm -rf $(BUILD_DIR)
```

#### `ForgeOS/linker/linker.ld`

```ld
ENTRY(_start)

SECTIONS
{
    . = 1M;

    .multiboot2_header : ALIGN(8)
    {
        KEEP(*(.multiboot2))
    }

    .text : ALIGN(4K)
    {
        *(.text)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}
```

#### `ForgeOS/boot/multiboot2_header.asm`

```nasm
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
```

#### `ForgeOS/boot/entry.asm`

```nasm
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
```

#### `ForgeOS/include/types.h`

```cpp
#ifndef TYPES_H
#define TYPES_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef long long int64_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;

typedef uint64_t size_t;
typedef int64_t ssize_t;
typedef int64_t ptrdiff_t;

#define NULL 0

#endif
```

#### `ForgeOS/include/io.h`

```cpp
#ifndef IO_H
#define IO_H

#include "types.h"

namespace Forge {
    namespace Console {
        enum Color {
            Black = 0,
            Blue = 1,
            Green = 2,
            Cyan = 3,
            Red = 4,
            Magenta = 5,
            Brown = 6,
            LightGrey = 7,
            DarkGrey = 8,
            LightBlue = 9,
            LightGreen = 10,
            LightCyan = 11,
            LightRed = 12,
            LightMagenta = 13,
            LightBrown = 14,
            White = 15
        };

        void init();
        void clear();
        void set_color(Color fg, Color bg);
        void put_char(char c);
        void print(const char* str);
        void println(const char* str);
    }
}

#endif
```

#### `ForgeOS/kernel/io.cpp`

```cpp
#include "io.h"

namespace Forge {
    namespace Console {
        volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;
        const size_t VGA_WIDTH = 80;
        const size_t VGA_HEIGHT = 25;

        size_t terminal_row;
        size_t terminal_column;
        uint8_t terminal_color;

        inline uint16_t vga_entry(char uc, uint8_t color) {
            uint16_t c = (uint8_t)uc;
            uint16_t col = color;
            return c | (col << 8);
        }

        void init() {
            terminal_row = 0;
            terminal_column = 0;
            terminal_color = LightGrey | (Black << 4);
            clear();
        }

        void clear() {
            for (size_t y = 0; y < VGA_HEIGHT; y++) {
                for (size_t x = 0; x < VGA_WIDTH; x++) {
                    size_t index = y * VGA_WIDTH + x;
                    vga_buffer[index] = vga_entry(' ', terminal_color);
                }
            }
        }

        void set_color(Color fg, Color bg) {
            terminal_color = fg | (bg << 4);
        }

        void put_char(char c) {
            if (c == '\n') {
                terminal_row++;
                terminal_column = 0;
                if (terminal_row >= VGA_HEIGHT) {
                    terminal_row = 0;
                    clear();
                }
                return;
            }

            if (terminal_column >= VGA_WIDTH) {
                terminal_column = 0;
                terminal_row++;
                if (terminal_row >= VGA_HEIGHT) {
                    terminal_row = 0;
                    clear();
                }
            }

            size_t index = terminal_row * VGA_WIDTH + terminal_column;
            vga_buffer[index] = vga_entry(c, terminal_color);
            terminal_column++;
        }

        void print(const char* str) {
            for (size_t i = 0; str[i] != '\0'; i++) {
                put_char(str[i]);
            }
        }

        void println(const char* str) {
            print(str);
            put_char('\n');
        }
    }
}
```

#### `ForgeOS/kernel/main.cpp`

```cpp
#include "io.h"

extern "C" void kernel_main() {
    Forge::Console::init();
    
    Forge::Console::set_color(Forge::Console::LightGreen, Forge::Console::Black);
    Forge::Console::println("ForgeOS v0.1");
    
    Forge::Console::set_color(Forge::Console::White, Forge::Console::Black);
    Forge::Console::println("Kernel initialized successfully.");
    Forge::Console::println("Entering Long Mode (64-bit)... OK.");
    Forge::Console::println("Welcome to ForgeOS!");

    while (true) {
        __asm__ __volatile__("hlt");
    }
}
```

#### `ForgeOS/scripts/grub.cfg`

```text
set timeout=0
set default=0

menuentry "ForgeOS" {
    multiboot2 /boot/forgeos.elf
    boot
}
```

#### `ForgeOS/.gitignore`

```text
build/
*.iso
*.elf
*.o
*.bin
*.log
*.swp
*~
```

#### `ForgeOS/README.md`

```markdown
# ForgeOS

**ForgeOS** é um sistema operacional x86_64 construído do zero, focado em aprendizado, experimentação e arquitetura limpa.

## Objetivos
- Construir um kernel real sem dependências de sistemas hospedeiros.
- Evoluir gradualmente de um boot simples até um sistema com userland, filesystem e GUI.
- Servir como material de estudo profundo sobre arquitetura de computadores e sistemas operacionais.

## Requisitos
- **Host:** Fedora Linux 44 KDE Plasma Edition
- **Emulador:** QEMU (x86_64)
- **Linguagem:** C++ (Freestanding) + Assembly (NASM)

## Compilação
```bash
make
```

## Execução
```bash
make run
```

## Debugging
```bash
make debug
```

## Estrutura de Diretórios
- `boot/`: Código Assembly de inicialização e transição de modo.
- `kernel/`: Lógica principal do kernel em C++.
- `include/`: Headers e tipos de dados isolados.
- `linker/`: Script de linker para mapeamento de memória.
- `scripts/`: Configurações do GRUB2.

## Roadmap
Veja a seção de Roadmap abaixo para as fases planejadas.

## Licença
MIT License. Veja [LICENSE](LICENSE) para mais detalhes.
