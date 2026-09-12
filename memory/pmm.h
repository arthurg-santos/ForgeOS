#ifndef PMM_H
#define PMM_H

#include "types.h"

namespace Forge {
    namespace Memory {
        constexpr uint64_t PAGE_SIZE = 4096;

        void pmm_init(uint64_t multiboot_info_addr);
        uint64_t pmm_alloc_page();
        void pmm_free_page(uint64_t addr);
        uint64_t pmm_total_pages();
        uint64_t pmm_free_pages();
    }
}

#endif
