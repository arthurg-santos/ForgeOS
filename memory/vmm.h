#ifndef VMM_H
#define VMM_H

#include "types.h"

namespace Forge {
    namespace Memory {
        constexpr uint64_t VMM_PRESENT  = 1ULL << 0;
        constexpr uint64_t VMM_WRITABLE = 1ULL << 1;
        constexpr uint64_t VMM_USER     = 1ULL << 2;

        void vmm_init();
        bool vmm_map_page(uint64_t va, uint64_t pa, uint64_t flags);
        bool vmm_unmap_page(uint64_t va);
        uint64_t vmm_get_phys(uint64_t va);
        uint64_t vmm_current_pml4();

        // Fase 7: espaços de endereço isolados por processo
        uint64_t vmm_new_address_space();
        bool vmm_map_page_in(uint64_t pml4_phys, uint64_t va, uint64_t pa, uint64_t flags);
        uint64_t vmm_get_phys_in(uint64_t pml4_phys, uint64_t va);
        void vmm_load_cr3(uint64_t pml4_phys);
    }
}

#endif
