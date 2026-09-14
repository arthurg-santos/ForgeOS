#ifndef USER_SYSCALLS_H
#define USER_SYSCALLS_H

#include "../../include/types.h"
#include "../../include/sysinfo.h"

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
constexpr uint64_t SYS_OPEN     = 10;
constexpr uint64_t SYS_READ     = 11;
constexpr uint64_t SYS_CLOSE    = 12;
constexpr uint64_t SYS_LS       = 13;
constexpr uint64_t SYS_RM       = 14;
constexpr uint64_t SYS_FWRITE   = 15;

constexpr uint64_t COL_BLACK = 0, COL_BLUE = 1, COL_GREEN = 2, COL_CYAN = 3;
constexpr uint64_t COL_RED = 4, COL_MAGENTA = 5, COL_BROWN = 6, COL_LGREY = 7;
constexpr uint64_t COL_DGREY = 8, COL_LBLUE = 9, COL_LGREEN = 10, COL_LCYAN = 11;
constexpr uint64_t COL_LRED = 12, COL_LMAGENTA = 13, COL_LBROWN = 14, COL_WHITE = 15;

constexpr int KEY_UP = 1, KEY_DOWN = 2, KEY_LEFT = 3, KEY_RIGHT = 4;
constexpr int KEY_HOME = 5, KEY_END = 6, KEY_DELETE = 7;

// Flags de open (espelham Forge::VFS)
constexpr int O_RDONLY = 0;
constexpr int O_WRONLY = 1;
constexpr int O_RDWR   = 2;
constexpr int O_CREATE = 4;

inline uint64_t syscall1(uint64_t num, uint64_t a0) {
    uint64_t ret;
    __asm__ __volatile__(
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "D"(a0)
        : "memory");
    return ret;
}

inline uint64_t syscall2(uint64_t num, uint64_t a0, uint64_t a1) {
    uint64_t ret;
    __asm__ __volatile__(
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "D"(a0), "S"(a1)
        : "memory");
    return ret;
}

inline uint64_t syscall3(uint64_t num, uint64_t a0, uint64_t a1, uint64_t a2) {
    uint64_t ret;
    __asm__ __volatile__(
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "D"(a0), "S"(a1), "d"(a2)
        : "memory");
    return ret;
}

inline void sys_write(const char* s) { syscall1(SYS_WRITE, (uint64_t)s); }
inline uint64_t sys_getpid() { return syscall1(SYS_GETPID, 0); }
inline void sys_yield() { syscall1(SYS_YIELD, 0); }
inline void sys_exit() { syscall1(SYS_EXIT, 0); }
inline uint64_t sys_readline(char* buf, uint64_t max) { return syscall2(SYS_READLINE, (uint64_t)buf, max); }
inline void sys_clear() { syscall1(SYS_CLEAR, 0); }
inline void sys_sysinfo(ForgeSysInfo* si) { syscall1(SYS_SYSINFO, (uint64_t)si); }
inline uint64_t sys_ps(uint64_t idx, char* buf) { return syscall2(SYS_PS, idx, (uint64_t)buf); }
inline void sys_setcolor(uint64_t fg, uint64_t bg) { syscall2(SYS_SETCOLOR, fg, bg); }
inline int sys_getchar() { return (int)syscall1(SYS_GETCHAR, 0); }

inline int sys_open(const char* name, int flags) { return (int)syscall2(SYS_OPEN, (uint64_t)name, (uint64_t)flags); }
inline int sys_read(int fd, uint8_t* buf, uint32_t n) { return (int)syscall3(SYS_READ, (uint64_t)fd, (uint64_t)buf, n); }
inline int sys_fwrite(int fd, const uint8_t* buf, uint32_t n) { return (int)syscall3(SYS_FWRITE, (uint64_t)fd, (uint64_t)buf, n); }
inline int sys_close(int fd) { return (int)syscall1(SYS_CLOSE, (uint64_t)fd); }
inline int sys_ls(char* buf, uint32_t maxlen) { return (int)syscall2(SYS_LS, (uint64_t)buf, maxlen); }
inline int sys_rm(const char* name) { return (int)syscall1(SYS_RM, (uint64_t)name); }

inline void con_left()  { sys_write("\x01"); }
inline void con_right() { sys_write("\x02"); }

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
