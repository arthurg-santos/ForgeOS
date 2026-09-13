#ifndef KHEAP_H
#define KHEAP_H

#include "types.h"

namespace Forge {
    namespace Memory {
        void kheap_init();
        void* kmalloc(uint64_t size);
        void kfree(void* ptr);
        uint64_t kheap_used();
        uint64_t kheap_capacity();
    }
}

#endif
