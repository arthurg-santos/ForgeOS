#ifndef SYSINFO_H
#define SYSINFO_H

#include "types.h"

struct ForgeSysInfo {
    uint64_t ticks;
    uint64_t total_kb;
    uint64_t free_kb;
    uint64_t tasks;
    uint64_t heap_used;
    uint64_t heap_cap;
};

#endif
