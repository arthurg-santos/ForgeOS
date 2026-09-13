#include "user_program.h"
#include "syscall.h"

namespace {
    // Wrapper de syscall: número em rax, arg0 em rdi, retorno em rax.
    inline uint64_t syscall1(uint64_t num, uint64_t a0) {
        uint64_t ret;
        __asm__ __volatile__(
            "int $0x80"
            : "=a"(ret)
            : "a"(num), "D"(a0)
            : "memory");
        return ret;
    }

    void sys_write(const char* s) { syscall1(Forge::Syscall::SYS_WRITE, (uint64_t)s); }
    uint64_t sys_getpid() { return syscall1(Forge::Syscall::SYS_GETPID, 0); }
    void sys_yield() { syscall1(Forge::Syscall::SYS_YIELD, 0); }
    void sys_exit() { syscall1(Forge::Syscall::SYS_EXIT, 0); }

    void put_dec(uint64_t v, char* buf) {
        buf[20] = '\0';
        int i = 19;
        if (v == 0) buf[i--] = '0';
        while (v > 0) { buf[i--] = (char)('0' + (v % 10)); v /= 10; }
        sys_write(&buf[i + 1]);
    }

    void run_loop(const char* name) {
        char buf[24];
        for (uint64_t tick = 1; tick <= 4; tick++) {
            sys_write("  [User ");
            sys_write(name);
            sys_write(" pid=");
            put_dec(sys_getpid(), buf);
            sys_write("] tick ");
            put_dec(tick, buf);
            sys_write("  (ring 3)\n");
            sys_yield();
        }
        sys_write("  [User ");
        sys_write(name);
        sys_write("] exiting via syscall\n");
        sys_exit();
    }
}

namespace Forge {
    namespace User {
        void user_main_a() { run_loop("A"); }
        void user_main_b() { run_loop("B"); }
    }
}
