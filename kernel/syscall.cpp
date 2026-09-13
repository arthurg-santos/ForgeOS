#include "syscall.h"
#include "io.h"
#include "scheduler.h"

namespace Forge {
    namespace Syscall {
        // ABI: rax = número, rdi = arg0. Retorno em rax (via frame).
        void syscall_handler(Interrupts::InterruptFrame* frame) {
            uint64_t num = frame->rax;
            uint64_t a0  = frame->rdi;
            uint64_t ret = 0;

            switch (num) {
                case SYS_WRITE: {
                    // Seção crítica: uma escrita por vez, sem preempção no meio
                    __asm__ __volatile__("cli");
                    Console::print((const char*)a0);
                    __asm__ __volatile__("sti");
                    ret = 0;
                    break;
                }
                case SYS_GETPID:
                    ret = Kernel::current_task_id();
                    break;
                case SYS_YIELD:
                    Kernel::yield();
                    break;
                case SYS_EXIT:
                    Kernel::task_exit(); // não retorna
                    break;
                default:
                    ret = (uint64_t)-1;
                    break;
            }

            frame->rax = ret; // valor visto pelo user após o iretq
        }
    }
}
