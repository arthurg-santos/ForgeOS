#include "elf.h"
#include "pmm.h"
#include "vmm.h"
#include "serial.h"

namespace Forge {
    namespace Elf {
        uint64_t elf_load(uint64_t pml4_phys, const uint8_t* blob, uint64_t size) {
            if (size < sizeof(Elf64_Ehdr)) return 0;
            const Elf64_Ehdr* e = (const Elf64_Ehdr*)blob;

            if (e->e_ident[0] != 0x7F || e->e_ident[1] != 'E' ||
                e->e_ident[2] != 'L'  || e->e_ident[3] != 'F') return 0;
            if (e->e_ident[4] != 2)      return 0;   // ELF64
            if (e->e_type != 2)          return 0;   // ET_EXEC
            if (e->e_machine != 0x3E)    return 0;   // x86-64
            if (e->e_phoff + (uint64_t)e->e_phnum * e->e_phentsize > size) return 0;

            for (uint16_t i = 0; i < e->e_phnum; i++) {
                const Elf64_Phdr* p =
                (const Elf64_Phdr*)(blob + e->e_phoff + (uint64_t)i * e->e_phentsize);
                if (p->p_type != PT_LOAD || p->p_memsz == 0) continue;

                uint64_t flags = Memory::VMM_PRESENT | Memory::VMM_USER;
                if (p->p_flags & PF_W) flags |= Memory::VMM_WRITABLE;

                uint64_t va_begin = p->p_vaddr & ~0xFFFULL;
                uint64_t va_end   = (p->p_vaddr + p->p_memsz + 0xFFF) & ~0xFFFULL;

                for (uint64_t va = va_begin; va < va_end; va += 4096) {
                    uint64_t pa = Memory::pmm_alloc_page();
                    if (pa == 0) return 0;

                    // Zera a página (kernel a acessa via identity map)
                    uint8_t* dst = (uint8_t*)pa;
                    for (int z = 0; z < 4096; z++) dst[z] = 0;

                    if (!Memory::vmm_map_page_in(pml4_phys, va, pa, flags)) return 0;

                    // Copia o trecho do arquivo que sobrepõe esta página
                    uint64_t seg_begin = p->p_vaddr;
                    uint64_t file_end  = p->p_offset + p->p_filesz;
                    uint64_t cp_start  = (seg_begin > va) ? seg_begin : va;
                    uint64_t cp_end    = (seg_begin + p->p_filesz < va + 4096)
                    ? (seg_begin + p->p_filesz) : (va + 4096);
                    if (cp_end > cp_start && cp_start < seg_begin + p->p_filesz) {
                        uint64_t src_off = p->p_offset + (cp_start - seg_begin);
                        if (src_off + (cp_end - cp_start) > file_end) {
                            cp_end = cp_start; // paranoia: nunca lê além do blob
                        } else {
                            uint8_t* d = (uint8_t*)(pa + (cp_start - va));
                            const uint8_t* s = blob + src_off;
                            for (uint64_t k = 0; k < cp_end - cp_start; k++) d[k] = s[k];
                        }
                    }
                }
            }

            Interrupts::klog_hex("elf: loaded, entry", e->e_entry);
            return e->e_entry;
        }
    }
}
