#ifndef ELF_H
#define ELF_H

#include "types.h"

namespace Forge {
    namespace Elf {
        struct Elf64_Ehdr {
            unsigned char e_ident[16];
            uint16_t e_type;
            uint16_t e_machine;
            uint32_t e_version;
            uint64_t e_entry;
            uint64_t e_phoff;
            uint64_t e_shoff;
            uint32_t e_flags;
            uint16_t e_ehsize;
            uint16_t e_phentsize;
            uint16_t e_phnum;
            uint16_t e_shentsize;
            uint16_t e_shnum;
            uint16_t e_shstrndx;
        } __attribute__((packed));

        struct Elf64_Phdr {
            uint32_t p_type;
            uint32_t p_flags;
            uint64_t p_offset;
            uint64_t p_vaddr;
            uint64_t p_paddr;
            uint64_t p_filesz;
            uint64_t p_memsz;
            uint64_t p_align;
        } __attribute__((packed));

        constexpr uint32_t PT_LOAD = 1;
        constexpr uint32_t PF_W = 2;

        // Carrega um ELF executável no address space dado.
        // Retorna o entry point, ou 0 em falha.
        uint64_t elf_load(uint64_t pml4_phys, const uint8_t* blob, uint64_t size);
    }
}

#endif
