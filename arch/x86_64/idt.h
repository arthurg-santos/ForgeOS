// idt.h
#ifndef IDT_H
#define IDT_H

#include "types.h"

namespace Forge {
    namespace Interrupts {
        struct InterruptFrame {
            uint64_t rax, rbx, rcx, rdx, rbp, rdi, rsi, r8, r9, r10, r11, r12, r13, r14, r15;
            uint64_t int_no, err_code;
            uint64_t rip, cs, rflags, rsp, ss;
        };

        void idt_init();
        void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags);
    }
}

#endif
