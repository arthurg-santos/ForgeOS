#ifndef SYSCALL_H
#define SYSCALL_H

#include "idt.h"

namespace Forge {
    namespace Syscall {
        constexpr uint64_t SYS_WRITE  = 0;
        constexpr uint64_t SYS_GETPID = 1;
        constexpr uint64_t SYS_YIELD  = 2;
        constexpr uint64_t SYS_EXIT   = 3;

        void syscall_handler(Interrupts::InterruptFrame* frame);
    }
}

#endif
