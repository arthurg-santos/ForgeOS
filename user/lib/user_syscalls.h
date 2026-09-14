#ifndef USER_SYSCALLS_H
#define USER_SYSCALLS_H

#include "../../include/types.h"

// ABI de syscall do ForgeOS: rax = número, rdi = arg0, retorno em rax.
constexpr uint64_t SYS_WRITE  = 0;
constexpr uint64_t SYS_GETPID = 1;
constexpr uint64_t SYS_YIELD  = 2;
constexpr uint64_t SYS_EXIT   = 3;

inline uint64_t syscall1(uint64_t num, uint64_t a0) {
    uint64_t ret;
    __asm__ __volatile__(
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "D"(a0)
        : "memory");
    return ret;
}

inline void sys_write(const char* s) { syscall1(SYS_WRITE, (uint64_t)s); }
inline uint64_t sys_getpid() { return syscall1(SYS_GETPID, 0); }
inline void sys_yield() { syscall1(SYS_YIELD, 0); }
inline void sys_exit() { syscall1(SYS_EXIT, 0); }

inline void put_dec(uint64_t v, char* buf) {
    buf[20] = '\0';
    int i = 19;
    if (v == 0) buf[i--] = '0';
    while (v > 0) { buf[i--] = (char)('0' + (v % 10)); v /= 10; }
    sys_write(&buf[i + 1]);
}

inline void put_hex(uint64_t v, char* buf) {
    const char* hex = "0123456789ABCDEF";
    buf[18] = '\0';
    buf[0] = '0'; buf[1] = 'x';
    for (int i = 17; i >= 2; i--) { buf[i] = hex[v & 0xF]; v >>= 4; }
    sys_write(buf);
}

#endif
