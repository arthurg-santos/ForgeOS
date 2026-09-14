#include "syscall.h"
#include "io.h"
#include "scheduler.h"
#include "keyboard.h"
#include "timer.h"
#include "kheap.h"
#include "pmm.h"
#include "sysinfo.h"

namespace Forge {
    namespace Syscall {
        namespace {
            void str_cat(char* dst, const char* src) {
                while (*dst) dst++;
                while ((*dst = *src)) { dst++; src++; }
            }
            void num_cat(char* dst, uint64_t v) {
                char t[24];
                t[20] = '\0';
                int i = 19;
                if (v == 0) t[i--] = '0';
                while (v > 0) { t[i--] = (char)('0' + (v % 10)); v /= 10; }
                str_cat(dst, &t[i + 1]);
            }
        }

        // ABI: rax = número, rdi = arg0, rsi = arg1. Retorno em rax (via frame).
        void syscall_handler(Interrupts::InterruptFrame* frame) {
            uint64_t num = frame->rax;
            uint64_t a0  = frame->rdi;
            uint64_t a1  = frame->rsi;
            uint64_t ret = 0;

            switch (num) {
                case SYS_WRITE: {
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
                case SYS_READLINE: {
                    char* ub = (char*)a0;
                    uint64_t max = a1;
                    uint64_t len = 0;
                    while (true) {
                        int c = Drivers::keyboard_get_char();
                        if (c < 0) { Kernel::yield(); continue; }
                        if (c == '\n') { Console::put_char('\n'); break; }
                        if (c == '\b') {
                            if (len > 0) { len--; Console::put_char('\b'); }
                            continue;
                        }
                        if (c >= 1 && c <= 7) continue; // teclas de edição: ignoradas aqui
                        if (len + 1 < max) {
                            ub[len++] = (char)c;
                            Console::put_char((char)c);
                        }
                    }
                    ub[len] = '\0';
                    ret = len;
                    break;
                }
                case SYS_CLEAR:
                    Console::clear();
                    break;
                case SYS_SYSINFO: {
                    ForgeSysInfo* si = (ForgeSysInfo*)a0;
                    si->ticks     = Drivers::timer_ticks();
                    si->total_kb  = Memory::pmm_total_pages() * 4;
                    si->free_kb   = Memory::pmm_free_pages() * 4;
                    si->tasks     = Kernel::proc_count();
                    si->heap_used = Memory::kheap_used();
                    si->heap_cap  = Memory::kheap_capacity();
                    ret = 0;
                    break;
                }
                case SYS_PS: {
                    Kernel::ProcEntry e;
                    if (!Kernel::proc_snapshot((int)a0, &e)) { ret = 1; break; }
                    char* buf = (char*)a1;
                    buf[0] = '\0';
                    str_cat(buf, "pid ");
                    num_cat(buf, e.id);
                    str_cat(buf, " [");
                    str_cat(buf, e.name);
                    str_cat(buf, "] ");
                    str_cat(buf, e.is_user ? "user " : "kern ");
                    str_cat(buf, e.alive ? "alive" : "dead");
                    ret = 0;
                    break;
                }
                case SYS_SETCOLOR:
                    Console::set_color((Console::Color)a0, (Console::Color)a1);
                    break;
                case SYS_GETCHAR:
                    // Não-bloqueante: -1 se não houver tecla no buffer
                    ret = (uint64_t)(int64_t)Drivers::keyboard_get_char();
                    break;
                default:
                    ret = (uint64_t)-1;
                    break;
            }

            frame->rax = ret;
        }
    }
}
