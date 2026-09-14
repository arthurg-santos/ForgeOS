#ifndef SYSCALL_H
#define SYSCALL_H

#include "idt.h"

namespace Forge {
    namespace Syscall {
        constexpr uint64_t SYS_WRITE    = 0;
        constexpr uint64_t SYS_GETPID   = 1;
        constexpr uint64_t SYS_YIELD    = 2;
        constexpr uint64_t SYS_EXIT     = 3;
        constexpr uint64_t SYS_READLINE = 4;
        constexpr uint64_t SYS_CLEAR    = 5;
        constexpr uint64_t SYS_SYSINFO  = 6;
        constexpr uint64_t SYS_PS       = 7;
        constexpr uint64_t SYS_SETCOLOR = 8;
        constexpr uint64_t SYS_GETCHAR  = 9;

        void syscall_handler(Interrupts::InterruptFrame* frame);
    }
}

#endif
