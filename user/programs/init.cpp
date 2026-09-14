#include "user_syscalls.h"

// Variável de dados em VA fixo do processo. Dois processos rodando o MESMO
// binário terão o MESMO &secret, mas páginas físicas DIFERENTES (isolamento).
static uint64_t secret = 0;

extern "C" void _start() {
    char buf[24];

    sys_write("init: ELF process online, pid ");
    put_dec(sys_getpid(), buf);
    sys_write("\n");

    sys_write("init: &secret = ");
    put_hex((uint64_t)&secret, buf);
    sys_write("\n");

    secret = sys_getpid() * 100;
    sys_write("init: secret = ");
    put_dec(secret, buf);
    sys_write(" (only mine)\n");

    for (uint64_t tick = 1; tick <= 3; tick++) {
        sys_write("init[pid ");
        put_dec(sys_getpid(), buf);
        sys_write("] tick ");
        put_dec(tick, buf);
        sys_write("\n");
        sys_yield();
    }

    sys_write("init[pid ");
    put_dec(sys_getpid(), buf);
    sys_write("] exiting\n");
    sys_exit();

    while (true) { }
}
