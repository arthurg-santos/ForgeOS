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
    }
}

#endif
