#ifndef GDT_H
#define GDT_H

#include "types.h"

namespace Forge {
    namespace CPU {
        void gdt_init();
        void set_tss_rsp0(uint64_t rsp0);
    }
}

#endif
